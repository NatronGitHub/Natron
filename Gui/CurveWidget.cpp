/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include "CurveWidget.h"

#include <cmath> // floor

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QTextStream>
#include <QtCore/QThread>
#include <QApplication>
#include <QToolButton>
#include <QDesktopWidget>

#include "Engine/ParameterWrapper.h" // IntParam
#include "Engine/Project.h"
#include "Engine/RotoContext.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidgetDialogs.h"
#include "Gui/CurveWidgetPrivate.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGraph.h"
#include "Gui/PythonPanels.h" // PyModelDialog
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

using namespace Natron;

/*****************************CURVE WIDGET***********************************************/


bool
CurveWidget::isSelectedKey(const boost::shared_ptr<CurveGui>& curve, double time) const
{
    for (SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        if ((*it)->curve == curve && time >= ((*it)->key.getTime() - 1e-6) && time <= ((*it)->key.getTime() + 1e-6)) {
            return true;
        }
    }
    return false;
}




void
CurveWidget::pushUndoCommand(QUndoCommand* cmd)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(cmd);
}



///////////////////////////////////////////////////////////////////
// CurveWidget
//

CurveWidget::CurveWidget(Gui* gui,
                         CurveSelection* selection,
                         boost::shared_ptr<TimeLine> timeline,
                         QWidget* parent,
                         const QGLWidget* shareWidget)
    : QGLWidget(parent,shareWidget)
    , _imp( new CurveWidgetPrivate(gui,selection,timeline,this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    if (timeline) {
        boost::shared_ptr<Natron::Project> project = gui->getApp()->getProject();
        assert(project);
        QObject::connect( timeline.get(),SIGNAL( frameChanged(SequenceTime,int) ),this,SLOT( onTimeLineFrameChanged(SequenceTime,int) ) );
        QObject::connect( project.get(),SIGNAL( frameRangeChanged(int,int) ),this,SLOT( onTimeLineBoundariesChanged(int,int) ) );
        onTimeLineFrameChanged(timeline->currentFrame(), Natron::eValueChangedReasonNatronGuiEdited);
        
        double left,right;
        project->getFrameRange(&left, &right);
        onTimeLineBoundariesChanged(left, right);
    }
    
    if (parent->objectName() == "CurveEditorSplitter") {
        ///if this is the curve widget associated to the CurveEditor
        //        QDesktopWidget* desktop = QApplication::desktop();
        //        _imp->sizeH = desktop->screenGeometry().size();
        _imp->sizeH = QSize(10000,10000);

    } else {
        ///a random parametric param curve editor
        _imp->sizeH =  QSize(400,400);
    }
    

}

CurveWidget::~CurveWidget()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();
}

void
CurveWidget::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !glewIsSupported("GL_ARB_vertex_array_object "  // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                          ) ) {
        _imp->_hasOpenGLVAOSupport = false;
    }
}

void
CurveWidget::addCurveAndSetColor(const boost::shared_ptr<CurveGui>& curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //updateGL(); //force initializeGL to be called if it wasn't before.
    _imp->_curves.push_back(curve);
    curve->setColor(_imp->_nextCurveAddedColor);
    _imp->_nextCurveAddedColor.setHsv( _imp->_nextCurveAddedColor.hsvHue() + 60,
                                       _imp->_nextCurveAddedColor.hsvSaturation(),_imp->_nextCurveAddedColor.value() );

}

void
CurveWidget::removeCurve(CurveGui *curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( it->get() == curve ) {
            //remove all its keyframes from selected keys
            SelectedKeys copy;
            for (SelectedKeys::iterator it2 = _imp->_selectedKeyFrames.begin(); it2 != _imp->_selectedKeyFrames.end(); ++it2) {
                if ( (*it2)->curve != (*it) ) {
                    copy.push_back(*it2);
                }
            }
            _imp->_selectedKeyFrames = copy;
            _imp->_curves.erase(it);
            break;
        }
    }
}

void
CurveWidget::centerOn(const std::vector<boost::shared_ptr<CurveGui> > & curves)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( curves.empty() ) {
        return;
    }
    
    bool doCenter = false;
    RectD ret;
    for (U32 i = 0; i < curves.size(); ++i) {
        const boost::shared_ptr<CurveGui>& c = curves[i];
        
        KeyFrameSet keys = c->getKeyFrames();

        if (keys.empty()) {
            continue;
        }
        doCenter = true;
        double xmin = keys.begin()->getTime();
        double xmax = keys.rbegin()->getTime();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            double value = it2->getValue();
            if (value < ymin) {
                ymin = value;
            }
            if (value > ymax) {
                ymax = value;
            }
        }
        ret.merge(xmin,ymin,xmax,ymax);
    }
    ret.set_bottom(ret.bottom() - ret.height() / 10);
    ret.set_left(ret.left() - ret.width() / 10);
    ret.set_right(ret.right() + ret.width() / 10);
    ret.set_top(ret.top() + ret.height() / 10);
    if (doCenter) {
        centerOn( ret.left(), ret.right(), ret.bottom(), ret.top() );
    }
}

void
CurveWidget::showCurvesAndHideOthers(const std::vector<boost::shared_ptr<CurveGui> > & curves)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<boost::shared_ptr<CurveGui> >::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        std::vector<boost::shared_ptr<CurveGui> >::const_iterator it2 = std::find(curves.begin(), curves.end(), *it);

        if ( it2 != curves.end() ) {
            (*it)->setVisible(true);
        } else {
            (*it)->setVisible(false);
        }
    }
    onCurveChanged();
}

void
CurveWidget::onCurveChanged()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///check whether selected keyframes have changed
    SelectedKeys copy;
    ///we cannot use std::transform here because a keyframe might have disappeared from a curve
    ///hence the number of keyframes selected would decrease
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        
        KeyFrameSet set = (*it)->curve->getKeyFrames();
        KeyFrameSet::const_iterator found = Curve::findWithTime(set, (*it)->key.getTime());
        
        if (found != set.end()) {
            (*it)->key = *found;
            _imp->refreshKeyTangents(*it);
            copy.push_back(*it);
        }
    }
    _imp->_selectedKeyFrames = copy;
    update();
}

void
CurveWidget::getVisibleCurves(std::vector<boost::shared_ptr<CurveGui> >* curves) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<boost::shared_ptr<CurveGui> >::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            curves->push_back(*it);
        }
    }
}

void
CurveWidget::centerOn(double xmin,
                      double xmax,
                      double ymin,
                      double ymax)

{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->zoomCtx.fit(xmin, xmax, ymin, ymax);
    _imp->zoomOrPannedSinceLastFit = false;
    refreshDisplayedTangents();

    update();
}

/**
 * @brief Swap the OpenGL buffers.
 **/
void
CurveWidget::swapOpenGLBuffers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    swapBuffers();
}

/**
 * @brief Repaint
 **/
void
CurveWidget::redraw()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    update();
}

/**
 * @brief Returns the width and height of the viewport in window coordinates.
 **/
void
CurveWidget::getViewportSize(double &width,
                             double &height) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    width = this->width();
    height = this->height();
}

/**
 * @brief Returns the pixel scale of the viewport.
 **/
void
CurveWidget::getPixelScale(double & xScale,
                           double & yScale) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    xScale = _imp->zoomCtx.screenPixelWidth();
    yScale = _imp->zoomCtx.screenPixelHeight();
}

/**
 * @brief Returns the colour of the background (i.e: clear color) of the viewport.
 **/
void
CurveWidget::getBackgroundColour(double &r,
                                 double &g,
                                 double &b) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);

}

void
CurveWidget::saveOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack();
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glCheckProjectionStack();
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glCheckModelviewStack();
    glPushMatrix();

    // set defaults to work around OFX plugin bugs
    glEnable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

void
CurveWidget::restoreOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glBindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

void
CurveWidget::resizeGL(int width,
                      int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if (height == 0) {
        height = 1;
    }
    glViewport (0, 0, width, height);
    _imp->zoomCtx.setScreenSize(width, height);

    if (height == 1) {
        //don't do the following when the height of the widget is irrelevant
        return;
    }

    if (!_imp->zoomOrPannedSinceLastFit) {
        ///find out what are the selected curves and center on them
        std::vector<boost::shared_ptr<CurveGui> > curves;
        getVisibleCurves(&curves);
        if ( curves.empty() ) {
            centerOn(-10,500,-10,10);
        } else {
            centerOn(curves);
        }
    }
    
}

void
CurveWidget::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    glCheckError();
    if (_imp->zoomCtx.factor() <= 0) {
        return;
    }
    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomCtx.left();
    zoomRight = _imp->zoomCtx.right();
    zoomBottom = _imp->zoomCtx.bottom();
    zoomTop = _imp->zoomCtx.top();
    
    double bgR,bgG,bgB;
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&bgR, &bgG, &bgB);
    
    if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
        glClearColor(bgR,bgG,bgB,1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

        return;
    }

    {
        GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        GLProtectMatrix p(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);
        GLProtectMatrix m(GL_MODELVIEW);
        glLoadIdentity();
        glCheckError();

        glClearColor(bgR,bgG,bgB,1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

        _imp->drawScale();

        if (_imp->_timelineEnabled) {
            _imp->drawTimelineMarkers();
        }

        if (_imp->_drawSelectedKeyFramesBbox) {
            _imp->drawSelectedKeyFramesBbox();
        }

        _imp->drawCurves();

        if ( !_imp->_selectionRectangle.isNull() ) {
            _imp->drawSelectionRectangle();
        }
    } // GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
    glCheckError();
}

void
CurveWidget::renderText(double x,
                        double y,
                        const QString & text,
                        const QColor & color,
                        const QFont & font) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( text.isEmpty() ) {
        return;
    }

    double w = (double)width();
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom();
    double left = _imp->zoomCtx.left();
    double top =  _imp->zoomCtx.top();
    double right = _imp->zoomCtx.right();
    if (w <= 0 || h <= 0 || right <= left || top <= bottom) {
        return;
    }
    double scalex = (right-left) / w;
    double scaley = (top-bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font);
    glCheckError();
}

void
CurveWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    ///If the click is on a curve but not nearby a keyframe, add a keyframe
    
    
    std::pair<boost::shared_ptr<CurveGui> ,KeyFrame > selectedKey = _imp->isNearbyKeyFrame( e->pos() );
    std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr > selectedTan = _imp->isNearbyTangent( e->pos() );
    if (selectedKey.first || selectedTan.second) {
        return;
    }
    

    EditKeyFrameDialog::EditModeEnum mode = EditKeyFrameDialog::eEditModeKeyframePosition;
    
    KeyPtr selectedText;
    ///We're nearby a selected keyframe's text
    KeyPtr keyText = _imp->isNearbyKeyFrameText(e->pos());
    if (keyText) {
        selectedText = keyText;
    } else {
        std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr> tangentText = _imp->isNearbySelectedTangentText(e->pos());
        if (tangentText.second) {
            if (tangentText.first == MoveTangentCommand::eSelectedTangentLeft) {
                mode = EditKeyFrameDialog::eEditModeLeftDerivative;
            } else {
                mode = EditKeyFrameDialog::eEditModeRightDerivative;
            }
            selectedText = tangentText.second;
        }
    }
    

    
    
    if (selectedText) {
        EditKeyFrameDialog* dialog = new EditKeyFrameDialog(mode,this,selectedText,this);
        int  dialogW = dialog->sizeHint().width();
        
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        
        QPoint gP = e->globalPos();
        if (gP.x() > (screen.width() - dialogW)) {
            gP.rx() -= dialogW;
        }
        
        dialog->move(gP);
        
        ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
        ///it closes it.
        QObject::connect( dialog,SIGNAL( accepted() ),this,SLOT( onEditKeyFrameDialogFinished() ) );
        QObject::connect( dialog,SIGNAL( rejected() ),this,SLOT( onEditKeyFrameDialogFinished() ) );
        dialog->show();

        e->accept();
        return;
    }
    
    ///We're nearby a curve
    double xCurve,yCurve;
    Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
    if ( foundCurveNearby != _imp->_curves.end() ) {
        std::pair<double,double> yRange = (*foundCurveNearby)->getCurveYRange();
        if (yCurve < yRange.first || yCurve > yRange.second) {
            QString err =  tr(" Out of curve y range ") +
                    QString("[%1 - %2]").arg(yRange.first).arg(yRange.second) ;
            Natron::warningDialog("", err.toStdString());
            e->accept();
            return;
        }
        std::vector<KeyFrame> keys(1);
        if ((*foundCurveNearby)->getInternalCurve()->areKeyFramesTimeClampedToIntegers()) {
            xCurve = std::floor(xCurve + 0.5);
        } else if ((*foundCurveNearby)->getInternalCurve()->areKeyFramesValuesClampedToBooleans()) {
            xCurve = double((bool)xCurve);
        }
        keys[0] = KeyFrame(xCurve,yCurve);
        

        pushUndoCommand(new AddKeysCommand(this,*foundCurveNearby,keys));
        
    }
}

void
CurveWidget::onEditKeyFrameDialogFinished()
{
    EditKeyFrameDialog* dialog = qobject_cast<EditKeyFrameDialog*>( sender() );
    
    if (dialog) {
        //QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        dialog->deleteLater();
    }

}

//
// Decide what should be done in response to a mouse press.
// When the reason is found, process it and return.
// (this function has as many return points as there are reasons)
//
void
CurveWidget::mousePressEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setFocus();
    ////
    // right button: popup menu
    if ( buttonDownIsRight(e) ) {
        _imp->createMenu();
        _imp->_rightClickMenu->exec( mapToGlobal( e->pos() ) );
        _imp->_dragStartPoint = e->pos();
        // no need to set _imp->_lastMousePos
        // no need to set _imp->_dragStartPoint

        // no need to updateGL()
        e->accept();
        return;
    }
    
    ////
    // is the click near a curve?
    double xCurve,yCurve;
    Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos(),&xCurve, &yCurve );
    if (foundCurveNearby != _imp->_curves.end()) {
        
        if (modCASIsControlAlt(e)) {
            
            _imp->selectCurve(*foundCurveNearby);

            std::pair<double,double> yRange = (*foundCurveNearby)->getCurveYRange();
            if (yCurve < yRange.first || yCurve > yRange.second) {
                QString err =  tr(" Out of curve y range ") +
                        QString("[%1 - %2]").arg(yRange.first).arg(yRange.second) ;
                Natron::warningDialog("", err.toStdString());
                e->accept();
                return;
            }
            std::vector<KeyFrame> keys(1);
            keys[0] = KeyFrame(xCurve,yCurve);
            pushUndoCommand(new AddKeysCommand(this,*foundCurveNearby,keys));
            
            _imp->_drawSelectedKeyFramesBbox = false;
            _imp->_mustSetDragOrientation = true;
            _imp->_state = eEventStateDraggingKeys;
            setCursor( QCursor(Qt::CrossCursor) );
            
            _imp->_selectedKeyFrames.clear();
            
            KeyPtr selected(new SelectedKey(*foundCurveNearby,keys[0]));
            
            _imp->refreshKeyTangents(selected);
            
            //insert it into the _selectedKeyFrames
            _imp->insertSelectedKeyFrameConditionnaly(selected);
            
            _imp->_keyDragLastMovement.rx() = 0.;
            _imp->_keyDragLastMovement.ry() = 0.;
            _imp->_dragStartPoint = e->pos();
            _imp->_lastMousePos = e->pos();
            
            return;
        }
    }
    
    ////
    // middle button: scroll view
    if ( buttonDownIsMiddle(e) ) {
        _imp->_state = eEventStateDraggingView;
        _imp->_lastMousePos = e->pos();
        _imp->_dragStartPoint = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to updateGL()
        return;
    } else if (((e->buttons() & Qt::MiddleButton) &&
                (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
               ((e->buttons() & Qt::LeftButton) &&
                (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier)))) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->_state = eEventStateZooming;
        _imp->_lastMousePos = e->pos();
        _imp->_dragStartPoint = e->pos();
        return;
    }
    
    // is the click near the multiple-keyframes selection box center?
    if (_imp->_drawSelectedKeyFramesBbox) {
        
        bool caughtBbox = true;
        if (_imp->isNearbySelectedKeyFramesCrossWidget(e->pos())) {
            _imp->_state = eEventStateDraggingKeys;
        } else if (_imp->isNearbyBboxBtmLeft(e->pos())) {
            _imp->_state = eEventStateDraggingBtmLeftBbox;
        } else if (_imp->isNearbyBboxMidLeft(e->pos())) {
            _imp->_state = eEventStateDraggingMidLeftBbox;
        } else if (_imp->isNearbyBboxTopLeft(e->pos())) {
            _imp->_state = eEventStateDraggingTopLeftBbox;
        } else if (_imp->isNearbyBboxMidTop(e->pos())) {
            _imp->_state = eEventStateDraggingMidTopBbox;
        } else if (_imp->isNearbyBboxTopRight(e->pos())) {
            _imp->_state = eEventStateDraggingTopRightBbox;
        } else if (_imp->isNearbyBboxMidRight(e->pos())) {
            _imp->_state = eEventStateDraggingMidRightBbox;
        } else if (_imp->isNearbyBboxBtmRight(e->pos())) {
            _imp->_state = eEventStateDraggingBtmRightBbox;
        } else if (_imp->isNearbyBboxMidBtm(e->pos())) {
            _imp->_state = eEventStateDraggingMidBtmBbox;
        } else {
            caughtBbox = false;
        }
        if (caughtBbox) {
            _imp->_mustSetDragOrientation = true;
            _imp->_keyDragLastMovement.rx() = 0.;
            _imp->_keyDragLastMovement.ry() = 0.;
            _imp->_dragStartPoint = e->pos();
            _imp->_lastMousePos = e->pos();
            //no need to updateGL()
            return;
        }
    }
    ////
    // is the click near a keyframe manipulator?
    std::pair<boost::shared_ptr<CurveGui> ,KeyFrame > selectedKey = _imp->isNearbyKeyFrame( e->pos() );
    if (selectedKey.first) {
        _imp->_drawSelectedKeyFramesBbox = false;
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingKeys;
        setCursor( QCursor(Qt::CrossCursor) );

        if ( !modCASIsControl(e) ) {
            _imp->_selectedKeyFrames.clear();
        }
        KeyPtr selected ( new SelectedKey(selectedKey.first,selectedKey.second) );

        _imp->refreshKeyTangents(selected);

        //insert it into the _selectedKeyFrames
        _imp->insertSelectedKeyFrameConditionnaly(selected);

        _imp->_keyDragLastMovement.rx() = 0.;
        _imp->_keyDragLastMovement.ry() = 0.;
        _imp->_dragStartPoint = e->pos();
        _imp->_lastMousePos = e->pos();
        update(); // the keyframe changes color and the derivatives must be drawn
        return;
    }
    

    
    
    ////
    // is the click near a derivative manipulator?
    std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr > selectedTan = _imp->isNearbyTangent( e->pos() );

    //select the derivative only if it is not a constant keyframe
    if ( selectedTan.second && (selectedTan.second->key.getInterpolation() != eKeyframeTypeConstant) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingTangent;
        _imp->_selectedDerivative = selectedTan;
        _imp->_lastMousePos = e->pos();
        //no need to set _imp->_dragStartPoint
        update();

        return;
    }
    
    KeyPtr nearbyKeyText = _imp->isNearbyKeyFrameText(e->pos());
    if (nearbyKeyText) {
        return;
    }
    
    std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr> tangentText = _imp->isNearbySelectedTangentText(e->pos());
    if (tangentText.second) {
        return;
    }
    
    
    ////
    // is the click near the vertical current time marker?
    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingTimeline;
        _imp->_lastMousePos = e->pos();
        // no need to set _imp->_dragStartPoint
        // no need to updateGL()
        return;
    }

    // yes, select it and don't start any other action, the user can then do per-curve specific actions
    // like centering on it on the viewport or pasting previously copied keyframes.
    // This is kind of the last resort action before the default behaviour (which is to draw
    // a selection rectangle), because we'd rather select a keyframe than the nearby curve
    if (foundCurveNearby != _imp->_curves.end()) {
        _imp->selectCurve(*foundCurveNearby);
    }

    ////
    // default behaviour: unselect selected keyframes, if any, and start a new selection
    _imp->_drawSelectedKeyFramesBbox = false;
    if ( !modCASIsControl(e) ) {
        _imp->_selectedKeyFrames.clear();
    }
    _imp->_state = eEventStateSelecting;
    _imp->_lastMousePos = e->pos();
    _imp->_dragStartPoint = e->pos();
    update();
} // mousePressEvent

void
CurveWidget::mouseReleaseEvent(QMouseEvent*)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (_imp->_evaluateOnPenUp) {
        _imp->_evaluateOnPenUp = false;
        
        if (_imp->_state == eEventStateDraggingKeys) {
            std::map<KnobHolder*,bool> toEvaluate;
            std::list<boost::shared_ptr<RotoContext> > rotoToEvaluate;
            for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
                KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>((*it)->curve.get());
                BezierCPCurveGui* isBezierCurve = dynamic_cast<BezierCPCurveGui*>((*it)->curve.get());
                if (isKnobCurve) {
                    
                    if (!isKnobCurve->getKnobGui()) {
                        boost::shared_ptr<RotoContext> roto = isKnobCurve->getRotoContext();
                        assert(roto);
                        rotoToEvaluate.push_back(roto);
                    } else {
                        
                        boost::shared_ptr<KnobI> knob = isKnobCurve->getInternalKnob();
                        assert(knob);
                        KnobHolder* holder = knob->getHolder();
                        assert(holder);
                        std::map<KnobHolder*,bool>::iterator found = toEvaluate.find(holder);
                        bool evaluateOnChange = knob->getEvaluateOnChange();
                        if ( ( found != toEvaluate.end() ) && !found->second && evaluateOnChange ) {
                            found->second = true;
                        } else if ( found == toEvaluate.end() ) {
                            toEvaluate.insert( std::make_pair(holder,evaluateOnChange) );
                        }
                    }
                } else if (isBezierCurve) {
                    rotoToEvaluate.push_back(isBezierCurve->getRotoContext());
                }
            }
            for (std::map<KnobHolder*,bool>::iterator it = toEvaluate.begin(); it != toEvaluate.end(); ++it) {
                it->first->evaluate_public(NULL, it->second,Natron::eValueChangedReasonUserEdited);
            }
            for (std::list<boost::shared_ptr<RotoContext> >::iterator it = rotoToEvaluate.begin(); it != rotoToEvaluate.end(); ++it) {
                (*it)->evaluateChange();
            }
        } else if (_imp->_state == eEventStateDraggingTangent) {
            KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>(_imp->_selectedDerivative.second->curve.get());
            BezierCPCurveGui* isBezierCurve = dynamic_cast<BezierCPCurveGui*>(_imp->_selectedDerivative.second->curve.get());
            if (isKnobCurve) {
                if (!isKnobCurve->getKnobGui()) {
                    boost::shared_ptr<RotoContext> roto = isKnobCurve->getRotoContext();
                    assert(roto);
                    roto->evaluateChange();
                } else {
                    boost::shared_ptr<KnobI> toEvaluate = isKnobCurve->getInternalKnob();
                    assert(toEvaluate);
                    toEvaluate->getHolder()->evaluate_public(toEvaluate.get(), true,Natron::eValueChangedReasonUserEdited);
                }
            } else if (isBezierCurve) {
                isBezierCurve->getRotoContext()->evaluateChange();
            }
        }
    }

    EventStateEnum prevState = _imp->_state;
    _imp->_state = eEventStateNone;
    _imp->_selectionRectangle.setBottomRight( QPointF(0,0) );
    _imp->_selectionRectangle.setTopLeft( _imp->_selectionRectangle.bottomRight() );
    if (_imp->_selectedKeyFrames.size() > 1) {
        _imp->_drawSelectedKeyFramesBbox = true;
    }
    if (prevState == eEventStateDraggingTimeline) {
        if (_imp->_gui->isDraftRenderEnabled()) {
            _imp->_gui->setDraftRenderEnabled(false);
            bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
            if (autoProxyEnabled) {
                _imp->_gui->renderAllViewers();
            }
        }
    }
    
    if (prevState == eEventStateSelecting) { // should other cases be considered?
        update();
    }
}

void
CurveWidget::mouseMoveEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //setFocus();

    //set cursor depending on the situation

    //find out if there is a nearby  derivative handle
    std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr > selectedTan = _imp->isNearbyTangent( e->pos() );

    //if the selected keyframes rectangle is drawn and we're nearby the cross
    if ( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
        setCursor( QCursor(Qt::SizeAllCursor) );
    } else {
        //if there's a keyframe handle nearby
        std::pair<boost::shared_ptr<CurveGui> ,KeyFrame > selectedKey = _imp->isNearbyKeyFrame( e->pos() );

        //if there's a keyframe or derivative handle nearby set the cursor to cross
        if (selectedKey.first || selectedTan.second) {
            setCursor( QCursor(Qt::CrossCursor) );
        } else {
            
            KeyPtr keyframeText = _imp->isNearbyKeyFrameText(e->pos());
            if (keyframeText) {
                setCursor( QCursor(Qt::IBeamCursor));
            } else {
                
                
                std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr> tangentText = _imp->isNearbySelectedTangentText(e->pos());
                if (tangentText.second) {
                    setCursor( QCursor(Qt::IBeamCursor));
                } else {
                    
                    //if we're nearby a timeline polygon, set cursor to horizontal displacement
                    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
                        setCursor( QCursor(Qt::SizeHorCursor) );
                    } else {
                        //default case
                        unsetCursor();
                    }
                }
            }
        }
    }

    if (_imp->_state == eEventStateNone) {
        // nothing else to do
        QGLWidget::mouseMoveEvent(e);
        return;
    }

    // after this point , only mouse dragging situations are handled
    assert(_imp->_state != eEventStateNone);

    if (_imp->_mustSetDragOrientation) {
        QPointF diff(e->pos() - _imp->_dragStartPoint);
        double dist = diff.manhattanLength();
        if (dist > 5) {
            if ( std::abs( diff.x() ) > std::abs( diff.y() ) ) {
                _imp->_mouseDragOrientation.setX(1);
                _imp->_mouseDragOrientation.setY(0);
            } else {
                _imp->_mouseDragOrientation.setX(0);
                _imp->_mouseDragOrientation.setY(1);
            }
            _imp->_mustSetDragOrientation = false;
        }
    }

    QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(),e->y() );
    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->_lastMousePos.x(),_imp->_lastMousePos.y() );
    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );
    switch (_imp->_state) {
    case eEventStateDraggingView:
        _imp->zoomOrPannedSinceLastFit = true;
        _imp->zoomCtx.translate(dx, dy);

        // Synchronize the dope sheet editor and opened viewers
        if (_imp->_gui->isTripleSyncEnabled()) {
            _imp->updateDopeSheetViewFrameRange();
            _imp->_gui->centerOpenedViewersOn(_imp->zoomCtx.left(), _imp->zoomCtx.right());
        }
        break;

    case eEventStateDraggingKeys:
        if (!_imp->_mustSetDragOrientation) {
            if ( !_imp->_selectedKeyFrames.empty() ) {
                _imp->moveSelectedKeyFrames(oldClick_opengl,newClick_opengl);
            }
        }
        break;
    case eEventStateDraggingBtmLeftBbox:
    case eEventStateDraggingMidBtmBbox:
    case eEventStateDraggingBtmRightBbox:
    case eEventStateDraggingMidRightBbox:
    case eEventStateDraggingTopRightBbox:
    case eEventStateDraggingMidTopBbox:
    case eEventStateDraggingTopLeftBbox:
    case eEventStateDraggingMidLeftBbox:
        if ( !_imp->_selectedKeyFrames.empty() ) {
            _imp->transformSelectedKeyFrames(oldClick_opengl, newClick_opengl, modCASIsShift(e));
        }
        break;
    case eEventStateSelecting:
        _imp->refreshSelectionRectangle( (double)e->x(),(double)e->y() );
        break;

    case eEventStateDraggingTangent:
        _imp->moveSelectedTangent(newClick_opengl);
        break;

    case eEventStateDraggingTimeline:
        _imp->_gui->setDraftRenderEnabled(true);
        _imp->_gui->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        _imp->_timeline->seekFrame( (SequenceTime)newClick_opengl.x(), false, 0,  Natron::eTimelineChangeReasonCurveEditorSeek );
        break;
    case eEventStateZooming: {
        
        _imp->zoomOrPannedSinceLastFit = true;
        
        int deltaX = 2 * (e->x() - _imp->_lastMousePos.x());
        int deltaY = - 2 * (e->y() - _imp->_lastMousePos.y());
        // Wheel: zoom values and time, keep point under mouse
        
        
        const double zoomFactor_min = 0.0001;
        const double zoomFactor_max = 10000.;
        const double par_min = 0.0001;
        const double par_max = 10000.;
        double zoomFactor;
        double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX);
        double scaleFactorY = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaY);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( _imp->_dragStartPoint.x(), _imp->_dragStartPoint.y() );
        
        
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactorY;
        
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactorY = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactorY = zoomFactor / _imp->zoomCtx.factor();
        }
        
        double par = _imp->zoomCtx.aspectRatio() / scaleFactorY;
        if (par <= par_min) {
            par = par_min;
            scaleFactorY = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactorY = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactorY);
        
        // Alt + Wheel: zoom time only, keep point under mouse
        par = _imp->zoomCtx.aspectRatio() * scaleFactorX;
        if (par <= par_min) {
            par = par_min;
            scaleFactorX = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactorX = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactorX);
        
        if (_imp->_drawSelectedKeyFramesBbox) {
            refreshSelectedKeysBbox();
        }
        refreshDisplayedTangents();
        
        // Synchronize the dope sheet editor and opened viewers
        if (_imp->_gui->isTripleSyncEnabled()) {
            _imp->updateDopeSheetViewFrameRange();
            _imp->_gui->centerOpenedViewersOn(_imp->zoomCtx.left(), _imp->zoomCtx.right());
        }
    } break;
    case eEventStateNone:
        assert(0);
        break;
    }

    _imp->_lastMousePos = e->pos();

    update();
    QGLWidget::mouseMoveEvent(e);
} // mouseMoveEvent

void
CurveWidget::refreshSelectedKeysBbox()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    RectD keyFramesBbox;
    for (SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin();
         it != _imp->_selectedKeyFrames.end();
         ++it) {
        double x = (*it)->key.getTime();
        double y = (*it)->key.getValue();
        if ( it != _imp->_selectedKeyFrames.begin() ) {
            if ( x < keyFramesBbox.left() ) {
                keyFramesBbox.set_left(x);
            }
            if ( x > keyFramesBbox.right() ) {
                keyFramesBbox.set_right(x);
            }
            if ( y > keyFramesBbox.top() ) {
                keyFramesBbox.set_top(y);
            }
            if ( y < keyFramesBbox.bottom() ) {
                keyFramesBbox.set_bottom(y);
            }
        } else {
            keyFramesBbox.set_left(x);
            keyFramesBbox.set_right(x);
            keyFramesBbox.set_top(y);
            keyFramesBbox.set_bottom(y);
        }
    }
    QPointF topLeft( keyFramesBbox.left(),keyFramesBbox.top() );
    QPointF btmRight( keyFramesBbox.right(),keyFramesBbox.bottom() );
    _imp->_selectedKeyFramesBbox.setTopLeft(topLeft);
    _imp->_selectedKeyFramesBbox.setBottomRight(btmRight);

    QPointF middle( ( topLeft.x() + btmRight.x() ) / 2., ( topLeft.y() + btmRight.y() ) / 2. );
    QPointF middleWidgetCoord = toWidgetCoordinates( middle.x(),middle.y() );
    QPointF middleLeft = _imp->zoomCtx.toZoomCoordinates( middleWidgetCoord.x() - 20,middleWidgetCoord.y() );
    QPointF middleRight = _imp->zoomCtx.toZoomCoordinates( middleWidgetCoord.x() + 20,middleWidgetCoord.y() );
    QPointF middleTop = _imp->zoomCtx.toZoomCoordinates(middleWidgetCoord.x(),middleWidgetCoord.y() - 20);
    QPointF middleBottom = _imp->zoomCtx.toZoomCoordinates(middleWidgetCoord.x(),middleWidgetCoord.y() + 20);

    _imp->_selectedKeyFramesCrossHorizLine.setPoints(middleLeft,middleRight);
    _imp->_selectedKeyFramesCrossVertLine.setPoints(middleBottom,middleTop);
}

void
CurveWidget::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    const double zoomFactor_min = 0.0001;
    const double zoomFactor_max = 10000.;
    const double par_min = 0.0001;
    const double par_max = 10000.;
    double zoomFactor;
    double par;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( modCASIsControlShift(e) ) {
        _imp->zoomOrPannedSinceLastFit = true;
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        par = _imp->zoomCtx.aspectRatio() / scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else if ( modCASIsControl(e) ) {
        _imp->zoomOrPannedSinceLastFit = true;
        // Alt + Wheel: zoom time only, keep point under mouse
        par = _imp->zoomCtx.aspectRatio() * scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else {
        _imp->zoomOrPannedSinceLastFit = true;
        // Wheel: zoom values and time, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }

    if (_imp->_drawSelectedKeyFramesBbox) {
        refreshSelectedKeysBbox();
    }
    refreshDisplayedTangents();

    update();

    // Synchronize the dope sheet editor and opened viewers
    if (_imp->_gui->isTripleSyncEnabled()) {
        _imp->updateDopeSheetViewFrameRange();
        _imp->_gui->centerOpenedViewersOn(_imp->zoomCtx.left(), _imp->zoomCtx.right());
    }

} // wheelEvent

QPointF
CurveWidget::toZoomCoordinates(double x,
                               double y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->zoomCtx.toZoomCoordinates(x, y);
}

QPointF
CurveWidget::toWidgetCoordinates(double x,
                                 double y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->zoomCtx.toWidgetCoordinates(x, y);
}

QSize
CurveWidget::sizeHint() const
{
    return _imp->sizeH;
}

static TabWidget* findParentTabRecursive(QWidget* w)
{
    QWidget* parent = w->parentWidget();
    if (!parent) {
        return 0;
    }
    TabWidget* tab = dynamic_cast<TabWidget*>(parent);
    if (tab) {
        return tab;
    }
    return findParentTabRecursive(parent);
}

void
CurveWidget::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    
    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) {
        TabWidget* parentTab = findParentTabRecursive(this);
        if (parentTab) {
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
            QCoreApplication::postEvent(parentTab,ev);
        }

    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys, modifiers, key) ) {
        deleteSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorConstant, modifiers, key) ) {
        constantInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorLinear, modifiers, key) ) {
        linearInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSmooth, modifiers, key) ) {
        smoothForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCatmullrom, modifiers, key) ) {
        catmullromInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCubic, modifiers, key) ) {
        cubicInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorHorizontal, modifiers, key) ) {
        horizontalInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorBreak, modifiers, key) ) {
        breakDerivativesForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenter, modifiers, key) ) {
        frameSelectedCurve();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll, modifiers, key) ) {
        selectAllKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCopy, modifiers, key) ) {
        copySelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorPaste, modifiers, key) ) {
        pasteKeyFramesFromClipBoardToSelectedCurve();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomIn, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomOut, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else {
        QGLWidget::keyPressEvent(e);
    }
} // keyPressEvent

void
CurveWidget::enterEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QWidget* currentFocus = qApp->focusWidget();
    
    bool canSetFocus = !currentFocus ||
            dynamic_cast<ViewerGL*>(currentFocus) ||
            dynamic_cast<CurveWidget*>(currentFocus) ||
            dynamic_cast<Histogram*>(currentFocus) ||
            dynamic_cast<NodeGraph*>(currentFocus) ||
            dynamic_cast<QToolButton*>(currentFocus) ||
            currentFocus->objectName() == "Properties" ||
            currentFocus->objectName() == "tree" ||
            currentFocus->objectName() == "SettingsPanel" ||
            currentFocus->objectName() == "qt_tabwidget_tabbar";
    
    if (canSetFocus) {
        setFocus();
    }
    
    QGLWidget::enterEvent(e);
}

//struct RefreshTangent_functor{
//    CurveWidgetPrivate* _imp;
//
//    RefreshTangent_functor(CurveWidgetPrivate* imp): _imp(imp){}
//
//    SelectedKey operator()(SelectedKey key){
//        _imp->refreshKeyTangents(key);
//        return key;
//    }
//};

void
CurveWidget::refreshDisplayedTangents()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        _imp->refreshKeyTangents(*it);
    }
    update();
}

void
CurveWidget::setSelectedKeys(const SelectedKeys & keys)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_selectedKeyFrames = keys;
    refreshDisplayedTangents();
    refreshSelectedKeysBbox();
}

void
CurveWidget::refreshSelectedKeys()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    refreshDisplayedTangents();
    refreshSelectedKeysBbox();
}

void
CurveWidget::constantInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeConstant);
}

void
CurveWidget::linearInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeLinear);
}

void
CurveWidget::smoothForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
}

void
CurveWidget::catmullromInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
}

void
CurveWidget::cubicInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeCubic);
}

void
CurveWidget::horizontalInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
}

void
CurveWidget::breakDerivativesForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeBroken);
}

void
CurveWidget::deleteSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( _imp->_selectedKeyFrames.empty() ) {
        return;
    }

    _imp->_drawSelectedKeyFramesBbox = false;
    _imp->_selectedKeyFramesBbox.setBottomRight( QPointF(0,0) );
    _imp->_selectedKeyFramesBbox.setTopLeft( _imp->_selectedKeyFramesBbox.bottomRight() );

    //apply the same strategy than for moveSelectedKeyFrames()

    std::map<boost::shared_ptr<CurveGui> ,std::vector<KeyFrame> >  toRemove;
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        std::map<boost::shared_ptr<CurveGui> ,std::vector<KeyFrame> >::iterator found = toRemove.find((*it)->curve);
        if (found != toRemove.end()) {
            found->second.push_back((*it)->key);
        } else {
            std::vector<KeyFrame> keys;
            keys.push_back((*it)->key);
            toRemove.insert(std::make_pair((*it)->curve, keys) );
        }
    }

    pushUndoCommand( new RemoveKeysCommand(this,toRemove) );
    

    _imp->_selectedKeyFrames.clear();

    update();
}

void
CurveWidget::copySelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_keyFramesClipBoard.clear();
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        _imp->_keyFramesClipBoard.push_back( (*it)->key );
    }
}

void
CurveWidget::pasteKeyFramesFromClipBoardToSelectedCurve()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    boost::shared_ptr<CurveGui> curve;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isSelected() ) {
            curve = (*it);
            break;
        }
    }
    if (!curve) {
        warningDialog( QObject::tr("Curve Editor").toStdString(),QObject::tr("You must select a curve first.").toStdString() );

        return;
    }
    //this function will call updateGL() for us
    pushUndoCommand( new AddKeysCommand(this,curve, _imp->_keyFramesClipBoard) );
}


void
CurveWidget::selectAllKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_drawSelectedKeyFramesBbox = true;
    _imp->_selectedKeyFrames.clear();
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            KeyFrameSet keys = (*it)->getKeyFrames();
            for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                KeyPtr newSelected( new SelectedKey(*it,*it2) );
                _imp->refreshKeyTangents(newSelected);
                _imp->insertSelectedKeyFrameConditionnaly(newSelected);
            }
        }
    }
    refreshSelectedKeysBbox();
    update();
}

void
CurveWidget::loopSelectedCurve()
{
    CurveEditor* ce = 0;
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == "CurveEditor") {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (!ce) {
        return;
    }
    
    boost::shared_ptr<CurveGui> curve = ce->getSelectedCurve();
    if (!curve) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must select a curve first in the view.").toStdString() );
        return;
    }
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>(curve.get());
    assert(knobCurve);
    PyModalDialog dialog(_imp->_gui);
    boost::shared_ptr<IntParam> firstFrame(dialog.createIntParam("firstFrame", "First frame"));
    firstFrame->setAnimationEnabled(false);
    boost::shared_ptr<IntParam> lastFrame(dialog.createIntParam("lastFrame", "Last frame"));
    lastFrame->setAnimationEnabled(false);
    dialog.refreshUserParamsGUI();
    if (dialog.exec()) {
        int first = firstFrame->getValue();
        int last = lastFrame->getValue();
        std::stringstream ss;
        ss << "curve(((frame - " << first << ") % (" << last << " - " << first << " + 1)) + " << first << ", "<< knobCurve->getDimension() << ")";
        std::string script = ss.str();
        ce->setSelectedCurveExpression(script.c_str());
    }

}

void
CurveWidget::negateSelectedCurve()
{
    CurveEditor* ce = 0;
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == "CurveEditor") {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (!ce) {
        return;
    }
    boost::shared_ptr<CurveGui> curve = ce->getSelectedCurve();
    if (!curve) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must select a curve first in the view.").toStdString() );
        return;
    }
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>(curve.get());
    assert(knobCurve);
    std::stringstream ss;
    ss << "-curve(frame, " << knobCurve->getDimension() << ")";
    std::string script = ss.str();
    ce->setSelectedCurveExpression(script.c_str());
}

void
CurveWidget::reverseSelectedCurve()
{
    CurveEditor* ce = 0;
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == "CurveEditor") {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (!ce) {
        return;
    }
    boost::shared_ptr<CurveGui> curve = ce->getSelectedCurve();
    if (!curve) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must select a curve first in the view.").toStdString() );
        return;
    }
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>(curve.get());
    assert(knobCurve);
    std::stringstream ss;
    ss << "curve(-frame, " << knobCurve->getDimension() << ")";
    std::string script = ss.str();
    ce->setSelectedCurveExpression(script.c_str());
}

void
CurveWidget::frameSelectedCurve()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    std::vector<boost::shared_ptr<CurveGui> > selection;
    _imp->_selectionModel->getSelectedCurves(&selection);
    centerOn(selection);
    if (selection.empty()) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must select a curve first in the left pane.").toStdString() );
    }
}

void
CurveWidget::onTimeLineFrameChanged(SequenceTime,
                                    int /*reason*/)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if (!_imp->_gui || _imp->_gui->isGUIFrozen()) {
        return;
    }

    if (!_imp->_timelineEnabled) {
        _imp->_timelineEnabled = true;
    }
    _imp->refreshTimelinePositions();
    update();
}

bool
CurveWidget::isTabVisible() const
{
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == "CurveEditor") {
                TabWidget* tab = dynamic_cast<TabWidget*>( parentWidget()->parentWidget()->parentWidget() );
                if (tab) {
                    if ( tab->isFloatingWindowChild() ) {
                        return true;
                    }

                    return tab->currentWidget() == parent;
                }
            }
        }
    }

    return false;
}

void
CurveWidget::onTimeLineBoundariesChanged(int,int)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    update();
}

const QColor &
CurveWidget::getSelectedCurveColor() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->_selectedCurveColor;
}

const QFont &
CurveWidget::getFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return *_imp->_font;
}

const SelectedKeys &
CurveWidget::getSelectedKeyFrames() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->_selectedKeyFrames;
}

bool
CurveWidget::isSupportingOpenGLVAO() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->_hasOpenGLVAOSupport;
}

const QFont &
CurveWidget::getTextFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return *_imp->_font;
}

void
CurveWidget::centerOn(double xmin, double xmax)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->zoomCtx.fill(xmin, xmax, _imp->zoomCtx.bottom(), _imp->zoomCtx.top());

    update();
}

void
CurveWidget::getProjection(double *zoomLeft,
                           double *zoomBottom,
                           double *zoomFactor,
                           double *zoomAspectRatio) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomAspectRatio = _imp->zoomCtx.aspectRatio();
}

void
CurveWidget::setProjection(double zoomLeft,
                           double zoomBottom,
                           double zoomFactor,
                           double zoomAspectRatio)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
}

void
CurveWidget::onUpdateOnPenUpActionTriggered()
{
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    appPTR->getCurrentSettings()->setRenderOnEditingFinishedOnly(!updateOnPenUpOnly);
}

void
CurveWidget::focusInEvent(QFocusEvent* e)
{
    QGLWidget::focusInEvent(e);
}

void
CurveWidget::exportCurveToAscii()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<boost::shared_ptr<CurveGui> > curves;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>(it->get());
        if ( (*it)->isVisible() && isKnobCurve) {
            boost::shared_ptr<KnobI> knob = isKnobCurve->getInternalKnob();
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());
            if (isString) {
                warningDialog( tr("Curve Editor").toStdString(),tr("String curves cannot be imported/exported.").toStdString() );
                return;
            }
            curves.push_back(*it);
        }
    }
    if ( curves.empty() ) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must have a curve on the editor first.").toStdString() );

        return;
    }

    ImportExportCurveDialog dialog(true,curves,_imp->_gui,this);
    if ( dialog.exec() ) {
        double x = dialog.getXStart();
        double end = dialog.getXEnd();
        double incr = dialog.getXIncrement();
        std::map<int,boost::shared_ptr<CurveGui> > columns;
        dialog.getCurveColumns(&columns);

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            double endInt = std::floor(end);
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) || ( endInt != end) ) ) {
                warningDialog( tr("Curve Export").toStdString(),curves[i]->getName().toStdString() + tr(" doesn't support X values that are not integers.").toStdString() );

                return;
            }
        }

        assert( !columns.empty() );
        int columnsCount = columns.rbegin()->first + 1;

        ///setup the file
        QString name = dialog.getFilePath();
        QFile file(name);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream ts(&file);

        for (double i = x; i <= end; i += incr) {
            for (int c = 0; c < columnsCount; ++c) {
                std::map<int,boost::shared_ptr<CurveGui> >::const_iterator foundCurve = columns.find(c);
                if ( foundCurve != columns.end() ) {
                    QString str = QString::number(foundCurve->second->evaluate(true,i),'f',10);
                    ts << str;
                } else {
                    ts <<  0;
                }
                if (c < columnsCount - 1) {
                    ts << '_';
                }
            }
            ts << '\n';
        }


        ///close the file
        file.close();
    }
} // exportCurveToAscii

void
CurveWidget::importCurveFromAscii()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<boost::shared_ptr<CurveGui> > curves;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>(it->get());
        if ( (*it)->isVisible() && isKnobCurve ) {
            
            boost::shared_ptr<KnobI> knob = isKnobCurve->getInternalKnob();
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());
            if (isString) {
                warningDialog( tr("Curve Editor").toStdString(),tr("String curves cannot be imported/exported.").toStdString() );
                return;
            }
            
            curves.push_back(*it);
        }
    }
    if ( curves.empty() ) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must have a curve on the editor first.").toStdString() );

        return;
    }

    ImportExportCurveDialog dialog(false,curves,_imp->_gui,this);
    if ( dialog.exec() ) {
        QString filePath = dialog.getFilePath();
        if ( !QFile::exists(filePath) ) {
            warningDialog( tr("Curve Import").toStdString(),tr("File not found.").toStdString() );

            return;
        }

        double x = dialog.getXStart();
        double incr = dialog.getXIncrement();
        std::map<int,boost::shared_ptr<CurveGui> > columns;
        dialog.getCurveColumns(&columns);
        assert( !columns.empty() );

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) ) ) {
                warningDialog( tr("Curve Import").toStdString(),curves[i]->getName().toStdString() + tr(" doesn't support X values that are not integers.").toStdString() );

                return;
            }
        }

        QFile file( dialog.getFilePath() );
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        std::map<boost::shared_ptr<CurveGui>, std::vector<double> > curvesValues;
        ///scan the file to get the curve values
        while ( !ts.atEnd() ) {
            QString line = ts.readLine();
            if ( line.isEmpty() ) {
                continue;
            }
            int i = 0;
            std::vector<double> values;

            ///read the line to extract all values
            while ( i < line.size() ) {
                QString value;
                while ( i < line.size() && line.at(i) != QChar('_') ) {
                    value.push_back( line.at(i) );
                    ++i;
                }
                if ( i < line.size() ) {
                    if ( line.at(i) != QChar('_') ) {
                        errorDialog( tr("Curve Import").toStdString(),tr("The file could not be read.").toStdString() );

                        return;
                    }
                    ++i;
                }
                bool ok;
                double v = value.toDouble(&ok);
                if (!ok) {
                    errorDialog( tr("Curve Import").toStdString(),tr("The file could not be read.").toStdString() );

                    return;
                }
                values.push_back(v);
            }
            ///assert that the values count is greater than the number of curves provided by the user
            if ( values.size() < columns.size() ) {
                errorDialog( tr("Curve Import").toStdString(),tr("The file contains less curves than what you selected.").toStdString() );

                return;
            }

            for (std::map<int,boost::shared_ptr<CurveGui> >::const_iterator col = columns.begin(); col != columns.end(); ++col) {
                if ( col->first >= (int)values.size() ) {
                    errorDialog( tr("Curve Import").toStdString(),tr("One of the curve column index is not a valid index for the given file.").toStdString() );

                    return;
                }
                std::map<boost::shared_ptr<CurveGui> , std::vector<double> >::iterator foundCurve = curvesValues.find(col->second);
                if ( foundCurve != curvesValues.end() ) {
                    foundCurve->second.push_back(values[col->first]);
                } else {
                    std::vector<double> curveValues(1);
                    curveValues[0] = values[col->first];
                    curvesValues.insert( std::make_pair(col->second, curveValues) );
                }
            }
        }
        ///now restore the curves since we know what we read is valid
        for (std::map<boost::shared_ptr<CurveGui>, std::vector<double> >::const_iterator it = curvesValues.begin(); it != curvesValues.end(); ++it) {
            
            std::vector<KeyFrame> keys;
            const std::vector<double> & values = it->second;
            double xIndex = x;
            for (U32 i = 0; i < values.size(); ++i) {
                KeyFrame k(xIndex,values[i], 0., 0., Natron::eKeyframeTypeLinear);
                keys.push_back(k);
                xIndex += incr;
            }
            
            pushUndoCommand(new SetKeysCommand(this,it->first,keys));

            
        }
        _imp->_selectedKeyFrames.clear();
        update();
    }
} // importCurveFromAscii

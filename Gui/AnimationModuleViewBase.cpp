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

#include "AnimationModuleViewBase.h"

#include <QGLWidget>
#include <QApplication>

#include <QThread>
#include <QImage>

#include "Engine/image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/TimeLine.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleViewPrivateBase.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/Menu.h"
#include "Gui/NodeAnim.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/PythonPanels.h"
#include "Gui/TableItemAnim.h"

// in pixels
#define CLICK_DISTANCE_TOLERANCE 5
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8
#define BOUNDING_BOX_HANDLE_SIZE 4


NATRON_NAMESPACE_ENTER;



AnimationViewBase::AnimationViewBase(QWidget* parent)
: QGLWidget(parent)
, _imp()
{
    setMouseTracking(true);
}

AnimationViewBase::~AnimationViewBase()
{

}

void
AnimationViewBase::initialize(Gui* gui, const AnimationModuleBasePtr& model)
{
    if (_imp) {
        return;
    }
    initializeImplementation(gui, model, this);

    TimeLinePtr timeline = model->getTimeline();
    if (timeline) {
        ProjectPtr project = gui->getApp()->getProject();
        assert(project);
        QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimeLineFrameChanged(SequenceTime,int)) );
        QObject::connect( project.get(), SIGNAL(frameRangeChanged(int,int)), this, SLOT(update()) );
        onTimeLineFrameChanged(timeline->currentFrame(), eValueChangedReasonNatronGuiEdited);

    }
}

void
AnimationViewBase::initializeGL()
{
    appPTR->initializeOpenGLFunctionsOnce();

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    _imp->generateKeyframeTextures();
}

void
AnimationViewBase::resizeGL(int width,
                      int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    if (height == 0) {
        height = 1;
    }
    GL_GPU::glViewport (0, 0, width, height);

    // Width and height may be 0 when tearing off a viewer tab to another panel
    if ( (width > 0) && (height > 0) ) {
        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.setScreenSize(width, height);
    }

    if (height == 1) {
        //don't do the following when the height of the widget is irrelevant
        return;
    }

    if (!_imp->zoomOrPannedSinceLastFit) {
        centerOnAllItems();
    }
}


void
AnimationViewBase::paintGL()
{


    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError(GL_GPU);
    if (_imp->zoomCtx.factor() <= 0) {
        return;
    }

    _imp->drawnOnce = true;

    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomCtx.left();
    zoomRight = _imp->zoomCtx.right();
    zoomBottom = _imp->zoomCtx.bottom();
    zoomTop = _imp->zoomCtx.top();

    double bgR, bgG, bgB;
    getBackgroundColour(bgR, bgG, bgB);

    if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
        GL_GPU::glClearColor(bgR, bgG, bgB, 1.);
        GL_GPU::glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        return;
    }

    {
        //GLProtectAttrib<GL_GPU> a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        //GLProtectMatrix<GL_GPU> p(GL_PROJECTION);
        GL_GPU::glMatrixMode(GL_PROJECTION);
        GL_GPU::glLoadIdentity();
        GL_GPU::glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);
        //GLProtectMatrix<GL_GPU> m(GL_MODELVIEW);
        GL_GPU::glMatrixMode(GL_MODELVIEW);
        GL_GPU::glLoadIdentity();
        glCheckError(GL_GPU);

        GL_GPU::glClearColor(bgR, bgG, bgB, 1.);
        GL_GPU::glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        // Call virtual portion
        drawView();
    }
    glCheckError(GL_GPU);
} // paintGL

void
AnimationViewBase::centerOn(double xmin,
                                         double xmax,
                                         double ymin,
                                         double ymax)

{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        QMutexLocker k(&_imp->zoomCtxMutex);

        if ( (_imp->zoomCtx.screenWidth() > 0) && (_imp->zoomCtx.screenHeight() > 0) ) {
            _imp->zoomCtx.fill(xmin, xmax, ymin, ymax);
        }
        _imp->zoomOrPannedSinceLastFit = false;
    }
    update();
}

void
AnimationViewBase::centerOn(double xmin,
                                         double xmax)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        if ( (_imp->zoomCtx.screenWidth() > 0) && (_imp->zoomCtx.screenHeight() > 0) ) {

            _imp->zoomCtx.fill( xmin, xmax, _imp->zoomCtx.bottom(), _imp->zoomCtx.top() );
        }
    }
    update();
}




void
AnimationViewBase::swapOpenGLBuffers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    swapBuffers();
}


void
AnimationViewBase::redraw()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    update();
}

bool
AnimationViewBase::hasDrawnOnce() const
{
    return _imp->drawnOnce;
}

void
AnimationViewBase::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    QGLFormat f = format();
    *hasAlpha = f.alpha();
    int r = f.redBufferSize();
    if (r == -1) {
        r = 8;// taken from qgl.h
    }
    int g = f.greenBufferSize();
    if (g == -1) {
        g = 8;// taken from qgl.h
    }
    int b = f.blueBufferSize();
    if (b == -1) {
        b = 8;// taken from qgl.h
    }
    int size = r;
    size = std::min(size, g);
    size = std::min(size, b);
    *depthPerComponents = size;
}


void
AnimationViewBase::getViewportSize(double &width,
                             double &height) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    width = this->width();
    height = this->height();
}

void
AnimationViewBase::getPixelScale(double & xScale,
                           double & yScale) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    xScale = _imp->zoomCtx.screenPixelWidth();
    yScale = _imp->zoomCtx.screenPixelHeight();
}



RectD
AnimationViewBase::getViewportRect() const
{
    RectD bbox;
    {
        bbox.x1 = _imp->zoomCtx.left();
        bbox.y1 = _imp->zoomCtx.bottom();
        bbox.x2 = _imp->zoomCtx.right();
        bbox.y2 = _imp->zoomCtx.top();
    }

    return bbox;
}

void
AnimationViewBase::getCursorPosition(double& x,
                               double& y) const
{
    QPoint p = QCursor::pos();

    p = mapFromGlobal(p);
    QPointF mappedPos = toZoomCoordinates( p.x(), p.y() );
    x = mappedPos.x();
    y = mappedPos.y();
}

void
AnimationViewBase::saveOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack(GL_GPU);
    GL_GPU::glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack(GL_GPU);
    GL_GPU::glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    GL_GPU::glMatrixMode(GL_PROJECTION);
    glCheckProjectionStack(GL_GPU);
    GL_GPU::glPushMatrix();
    GL_GPU::glMatrixMode(GL_MODELVIEW);
    glCheckModelviewStack(GL_GPU);
    GL_GPU::glPushMatrix();

    // set defaults to work around OFX plugin bugs
    GL_GPU::glEnable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

void
AnimationViewBase::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::glBindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    GL_GPU::glMatrixMode(GL_PROJECTION);
    GL_GPU::glPopMatrix();
    GL_GPU::glMatrixMode(GL_MODELVIEW);
    GL_GPU::glPopMatrix();
    GL_GPU::glPopClientAttrib();
    GL_GPU::glPopAttrib();
}

bool
AnimationViewBase::renderText(double x,
                        double y,
                        const std::string &string,
                        double r,
                        double g,
                        double b,
                        int flags)
{
    QColor c;

    c.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    _imp->renderText( x, y, QString::fromUtf8( string.c_str() ), c, font(), flags );

    return true;
}


QPointF
AnimationViewBase::toZoomCoordinates(double x,
                                     double y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->zoomCtx.toZoomCoordinates(x, y);
}

QPointF
AnimationViewBase::toWidgetCoordinates(double x,
                                       double y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->zoomCtx.toWidgetCoordinates(x, y);
}

/**
 * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
 **/
void
AnimationViewBase::toWidgetCoordinates(double *x,
                                 double *y) const
{
    QPointF p = _imp->zoomCtx.toWidgetCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

/**
 * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
 **/
void
AnimationViewBase::toCanonicalCoordinates(double *x,
                                    double *y) const
{
    QPointF p = _imp->zoomCtx.toZoomCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

/**
 * @brief Returns the font height, i.e: the height of the highest letter for this font
 **/
int
AnimationViewBase::getWidgetFontHeight() const
{
    return fontMetrics().height();
}

/**
 * @brief Returns for a string the estimated pixel size it would take on the widget
 **/
int
AnimationViewBase::getStringWidthForCurrentFont(const std::string& string) const
{
    return fontMetrics().width( QString::fromUtf8( string.c_str() ) );
}



void
AnimationViewBase::getProjection(double *zoomLeft,
                           double *zoomBottom,
                           double *zoomFactor,
                           double *zoomAspectRatio) const
{
    QMutexLocker k(&_imp->zoomCtxMutex);

    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomAspectRatio = _imp->zoomCtx.aspectRatio();
}

void
AnimationViewBase::setProjection(double zoomLeft,
                           double zoomBottom,
                           double zoomFactor,
                           double zoomAspectRatio)
{
    // always running in the main thread
    QMutexLocker k(&_imp->zoomCtxMutex);

    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
}


void
AnimationViewBase::onTimeLineFrameChanged(SequenceTime,
                                    int /*reason*/)
{
    if ( !_imp->_gui || _imp->_gui->isGUIFrozen() ) {
        return;
    }

    if ( isVisible() ) {
        update();
    }
}


void
AnimationViewBase::onRemoveSelectedKeyFramesActionTriggered()
{
    _imp->_model.lock()->deleteSelectedKeyframes();
}

void
AnimationViewBase::onCopySelectedKeyFramesToClipBoardActionTriggered()
{
    _imp->_model.lock()->copySelectedKeys();
}

void
AnimationViewBase::onPasteClipBoardKeyFramesActionTriggered()
{
    AnimationModuleBasePtr model = _imp->_model.lock();
    model->pasteKeys(model->getKeyFramesClipBoard(), true /*relative*/);
}

void
AnimationViewBase::onPasteClipBoardKeyFramesAbsoluteActionTriggered()
{
    AnimationModuleBasePtr model = _imp->_model.lock();
    model->pasteKeys(model->getKeyFramesClipBoard(), false /*relative*/);
}

void
AnimationViewBase::onSelectAllKeyFramesActionTriggered()
{
    _imp->_model.lock()->getSelectionModel()->selectAll();
}

void
AnimationViewBase::onSetInterpolationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    KeyframeTypeEnum type = (KeyframeTypeEnum)action->data().toInt();
    _imp->_model.lock()->setSelectedKeysInterpolation(type);
}

void
AnimationViewBase::onCenterAllCurvesActionTriggered()
{
    centerOnAllItems();
    
}

void
AnimationViewBase::onCenterOnSelectedCurvesActionTriggered()
{
    centerOnSelection();
}

void
AnimationViewBase::onSelectionModelKeyframeSelectionChanged()
{
    refreshSelectionBboxAndRedraw();
}

void
AnimationViewBase::onUpdateOnPenUpActionTriggered()
{
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    appPTR->getCurrentSettings()->setRenderOnEditingFinishedOnly(!updateOnPenUpOnly);
}

void
AnimationViewBase::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool accept = true;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    AnimationModuleBasePtr model = _imp->_model.lock();
    if (!model) {
        return;
    }
    if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleRemoveKeys, modifiers, key) ) {
        onRemoveSelectedKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleConstant, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeConstant);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleLinear, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeLinear);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleSmooth, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCatmullrom, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCubic, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeCubic);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleHorizontal, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleBreak, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeBroken);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCenterAll, modifiers, key) ) {
        onCenterAllCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCenter, modifiers, key) ) {
        onCenterOnSelectedCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleSelectAll, modifiers, key) ) {
        onSelectAllKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCopy, modifiers, key) ) {
        onCopySelectedKeyFramesToClipBoardActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModulePasteKeyframes, modifiers, key) ) {
        onPasteClipBoardKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModulePasteKeyframesAbsolute, modifiers, key) ) {
        onPasteClipBoardKeyFramesAbsoluteActionTriggered();
    } else if ( key == Qt::Key_Plus ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( key == Qt::Key_Minus ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else {
        accept = false;
    }

    AnimationModulePtr isAnimModule = toAnimationModule(model);
    if (accept) {
        if (isAnimModule) {
            isAnimModule->getEditor()->onInputEventCalled();
        }

        e->accept();
    } else {
        if (isAnimModule) {
            isAnimModule->getEditor()->handleUnCaughtKeyPressEvent(e);
        }
        QGLWidget::keyPressEvent(e);
    }
} // keyPressEvent



NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_AnimationModuleViewBase.cpp"

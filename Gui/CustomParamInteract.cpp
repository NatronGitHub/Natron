/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "CustomParamInteract.h"

#include <stdexcept>

#include <QtCore/QSize>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QMouseEvent>
#include <QtCore/QByteArray>

#include "Engine/OverlayInteractBase.h"
#include "Engine/Knob.h"
#include "Engine/AppInstance.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/TimeLine.h"

#include "Gui/KnobGui.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/GuiApplicationManager.h"


NATRON_NAMESPACE_ENTER

struct CustomParamInteractPrivate
{
    KnobGuiWPtr knob;
    OverlayInteractBasePtr interact;
    QSize preferredSize;
    double par;
    GLuint savedTexture;

    CustomParamInteractPrivate(const KnobGuiPtr& knob,
                               const OverlayInteractBasePtr & interact)
        : knob(knob)
        , interact(interact)
        , preferredSize()
        , par(0)
        , savedTexture(0)
    {
        assert(interact);
        interact->getPixelAspectRatio(par);
        int pW, pH;
        interact->getPreferredSize(pW, pH);
        preferredSize.setWidth(pW);
        preferredSize.setHeight(pH);
    }
};

CustomParamInteract::CustomParamInteract(const KnobGuiPtr& knob,
                                         const OverlayInteractBasePtr & entryPoint,
                                         QWidget* parent)
    : QGLWidget(parent)
    , _imp( new CustomParamInteractPrivate(knob, entryPoint) )
{
    double minW, minH;

    entryPoint->getMinimumSize(minW, minH);
    setMinimumSize(minW, minH);
}

CustomParamInteract::~CustomParamInteract()
{
}

void
CustomParamInteract::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError(GL_GPU);

    /*
       http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParametersInteracts
       The GL_PROJECTION matrix will be an orthographic 2D view with -0.5,-0.5 at the bottom left and viewport width-0.5, viewport height-0.5 at the top right.

       The GL_MODELVIEW matrix will be the identity matrix.
     */
    {
        GLProtectAttrib<GL_GPU> a(GL_TRANSFORM_BIT);
        GLProtectMatrix<GL_GPU> p(GL_PROJECTION);
        GL_GPU::LoadIdentity();
        GL_GPU::Ortho(-0.5, width() - 0.5, -0.5, height() - 0.5, 1, -1);
        GLProtectMatrix<GL_GPU> m(GL_MODELVIEW);
        GL_GPU::LoadIdentity();

        /*A parameter's interact draw function will have full responsibility for drawing the interact, including clearing the background and swapping buffers.*/
        OfxPointD scale;
        scale.x = scale.y = 1.;
        TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());

        _imp->interact->drawOverlay_public(this, time, RenderScale(1.), ViewIdx(0));

        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_TRANSFORM_BIT);
}

void
CustomParamInteract::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    appPTR->initializeOpenGLFunctionsOnce();
}

void
CustomParamInteract::resizeGL(int w,
                              int h)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    if (h == 0) {
        h = 1;
    }
    GL_GPU::Viewport (0, 0, w, h);
    _imp->interact->setSize(w, h);
}

QSize
CustomParamInteract::sizeHint() const
{
    return _imp->preferredSize;
}

void
CustomParamInteract::swapOpenGLBuffers()
{
    swapBuffers();
}

void
CustomParamInteract::redraw()
{
    update();
}

void
CustomParamInteract::getViewportSize(double &w,
                                     double &h) const
{
    w = width();
    h = height();
}


void
CustomParamInteract::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
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
CustomParamInteract::getPixelScale(double & xScale,
                                   double & yScale) const
{
    xScale = 1.;
    yScale = 1.;
}

#ifdef OFX_EXTENSIONS_NATRON
double
CustomParamInteract::getScreenPixelRatio() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return windowHandle()->devicePixelRatio()
#else
    return 1.;
#endif
}
#endif

void
CustomParamInteract::getBackgroundColour(double &r,
                                         double &g,
                                         double &b) const
{
    r = 0;
    g = 0;
    b = 0;
}

void
CustomParamInteract::toWidgetCoordinates(double *x,
                                         double *y) const
{
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void
CustomParamInteract::toCanonicalCoordinates(double *x,
                                            double *y) const
{
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void
CustomParamInteract::saveOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack(GL_GPU);
    GL_GPU::PushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack(GL_GPU);
    GL_GPU::PushClientAttrib(GL_ALL_ATTRIB_BITS);
    GL_GPU::MatrixMode(GL_PROJECTION);
    glCheckProjectionStack(GL_GPU);
    GL_GPU::PushMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    glCheckModelviewStack(GL_GPU);
    GL_GPU::PushMatrix();

    // set defaults to work around OFX plugin bugs
    GL_GPU::Enable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

void
CustomParamInteract::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::BindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    GL_GPU::MatrixMode(GL_PROJECTION);
    GL_GPU::PopMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    GL_GPU::PopMatrix();
    GL_GPU::PopClientAttrib();
    GL_GPU::PopAttrib();
}

/**
 * @brief Returns the font height, i.e: the height of the highest letter for this font
 **/
int
CustomParamInteract::getWidgetFontHeight() const
{
    return fontMetrics().height();
}

/**
 * @brief Returns for a string the estimated pixel size it would take on the widget
 **/
int
CustomParamInteract::getStringWidthForCurrentFont(const std::string& string) const
{
    return fontMetrics().width( QString::fromUtf8( string.c_str() ) );
}

RectD
CustomParamInteract::getViewportRect() const
{
    RectD bbox;
    {
        bbox.x1 = -0.5;
        bbox.y1 = -0.5;
        bbox.x2 = width() + 0.5;
        bbox.y2 = height() + 0.5;
    }

    return bbox;
}

void
CustomParamInteract::getCursorPosition(double& x,
                                       double& y) const
{
    QPoint p = QCursor::pos();

    p = mapFromGlobal(p);
    x = p.x();
    y = p.y();
}

void
CustomParamInteract::mousePressEvent(QMouseEvent* e)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());
    QPointF pos;
    QPointF viewportPos;
    pos.rx() = e->x();
    pos.ry() = height() - 1 - e->y();
    viewportPos.rx() = pos.x();
    viewportPos.ry() = pos.y();

    bool caught = _imp->interact->onOverlayPenDown_public(this, time, RenderScale(1.), ViewIdx(0), viewportPos, pos, 1. /*pressure*/, TimeValue(0.) /*timestamp*/, ePenTypeLMB);
    if (caught) {
        update();
    }
}

void
CustomParamInteract::mouseMoveEvent(QMouseEvent* e)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());
    QPointF pos;
    QPointF viewportPos;
    pos.rx() = e->x();
    pos.ry() = height() - 1 - e->y();
    viewportPos.rx() = pos.x();
    viewportPos.ry() = pos.y();

    bool caught = _imp->interact->onOverlayPenMotion_public(this, time, RenderScale(1.), ViewIdx(0), viewportPos, pos, 1. /*pressure*/, TimeValue(0.) /*timestamp*/);

    if (caught) {
        update();
    }
}

void
CustomParamInteract::mouseReleaseEvent(QMouseEvent* e)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());
    QPointF pos;
    QPointF viewportPos;
    pos.rx() = e->x();
    pos.ry() = height() - 1 - e->y();
    viewportPos.rx() = pos.x();
    viewportPos.ry() = pos.y();

    bool caught = _imp->interact->onOverlayPenUp_public(this, time, RenderScale(1.), ViewIdx(0), viewportPos, pos, 1. /*pressure*/, TimeValue(0.) /*timestamp*/);
    if (caught) {
        update();
    }
}

void
CustomParamInteract::focusInEvent(QFocusEvent* /*e*/)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());

    bool caught = _imp->interact->onOverlayFocusGained_public(this, time, RenderScale(1.), ViewIdx(0));
    if (caught) {
        update();
    }
}

void
CustomParamInteract::focusOutEvent(QFocusEvent* /*e*/)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());
    bool caught = _imp->interact->onOverlayFocusLost_public(this, time, RenderScale(1.), ViewIdx(0));
    if (caught) {
        update();
    }
}

void
CustomParamInteract::keyPressEvent(QKeyEvent* e)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());
    QByteArray keyStr;
    bool caught;
    if ( e->isAutoRepeat() ) {
        caught = _imp->interact->onOverlayKeyDown_public(this, time, RenderScale(1.), ViewIdx(0), QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), QtEnumConvert::fromQtModifiers(e->modifiers()));
    } else {
        caught = _imp->interact->onOverlayKeyRepeat_public(this, time, RenderScale(1.), ViewIdx(0), QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), QtEnumConvert::fromQtModifiers(e->modifiers()));
    }
    if (caught) {
        update();
    }
}

void
CustomParamInteract::keyReleaseEvent(QKeyEvent* e)
{

    TimeValue time(_imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame());
    QByteArray keyStr;
    bool caught = _imp->interact->onOverlayKeyUp_public(this, time, RenderScale(1.), ViewIdx(0), QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), QtEnumConvert::fromQtModifiers(e->modifiers()));
    if (caught) {
        update();
    }
}

NATRON_NAMESPACE_EXIT

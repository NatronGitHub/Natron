/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "Gui/KnobGui.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/GuiApplicationManager.h"

#include "Engine/OfxOverlayInteract.h"
#include "Engine/Knob.h"
#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"


NATRON_NAMESPACE_ENTER

struct CustomParamInteractPrivate
{
    KnobGuiWPtr knob;
    OFX::Host::Param::Instance* ofxParam;
    OfxParamOverlayInteractPtr entryPoint;
    QSize preferredSize;
    double par;
    GLuint savedTexture;

    CustomParamInteractPrivate(const KnobGuiPtr& knob,
                               void* ofxParamHandle,
                               const OfxParamOverlayInteractPtr & entryPoint)
        : knob(knob)
        , ofxParam(0)
        , entryPoint(entryPoint)
        , preferredSize()
        , par(0)
        , savedTexture(0)
    {
        assert(entryPoint && ofxParamHandle);
        ofxParam = reinterpret_cast<OFX::Host::Param::Instance*>(ofxParamHandle);
        assert( ofxParam->verifyMagic() );

        par = entryPoint->getProperties().getIntProperty(kOfxParamPropInteractSizeAspect);
        int pW, pH;
        entryPoint->getPreferredSize(pW, pH);
        preferredSize.setWidth(pW);
        preferredSize.setHeight(pH);
    }
};

CustomParamInteract::CustomParamInteract(const KnobGuiPtr& knob,
                                         void* ofxParamHandle,
                                         const OfxParamOverlayInteractPtr & entryPoint,
                                         QWidget* parent)
    : QGLWidget(parent)
    , _imp( new CustomParamInteractPrivate(knob, ofxParamHandle, entryPoint) )
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


    glCheckError();

    /*
       http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParametersInteracts
       The GL_PROJECTION matrix will be an orthographic 2D view with -0.5,-0.5 at the bottom left and viewport width-0.5, viewport height-0.5 at the top right.

       The GL_MODELVIEW matrix will be the identity matrix.
     */
    {
        GLProtectAttrib a(GL_TRANSFORM_BIT);
        GLProtectMatrix p(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-0.5, width() - 0.5, -0.5, height() - 0.5, 1, -1);
        GLProtectMatrix m(GL_MODELVIEW);
        glLoadIdentity();

        /*A parameter's interact draw function will have full responsibility for drawing the interact, including clearing the background and swapping buffers.*/
        OfxPointD scale;
        scale.x = scale.y = 1.;
        double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
        _imp->entryPoint->drawAction(time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0);
        glCheckError();
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
    glViewport (0, 0, w, h);
    _imp->entryPoint->setSize(w, h);
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

    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack();
    glPushClientAttrib(GL_ALL_ATTRIB_BITS);
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
CustomParamInteract::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    glBindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
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
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxPointD pos;
    OfxPointI viewportPos;
    pos.x = e->x();
    pos.y = height() - 1 - e->y();
    viewportPos.x = pos.x;
    viewportPos.y = pos.y;
    OfxStatus stat = _imp->entryPoint->penDownAction(time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0, pos, viewportPos, /*pressure=*/ 1.);
    if (stat == kOfxStatOK) {
        update();
    }
}

void
CustomParamInteract::mouseMoveEvent(QMouseEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxPointD pos;
    OfxPointI viewportPos;
    pos.x = e->x();
    pos.y = height() - 1 - e->y();
    viewportPos.x = pos.x;
    viewportPos.y = pos.y;
    OfxStatus stat = _imp->entryPoint->penMotionAction(time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0, pos, viewportPos, /*pressure=*/ 1.);
    if (stat == kOfxStatOK) {
        update();
    }
}

void
CustomParamInteract::mouseReleaseEvent(QMouseEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxPointD pos;
    OfxPointI viewportPos;
    pos.x = e->x();
    pos.y = height() - 1 - e->y();
    viewportPos.x = pos.x;
    viewportPos.y = pos.y;
    OfxStatus stat = _imp->entryPoint->penUpAction(time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0, pos, viewportPos, /*pressure=*/ 1.);
    if (stat == kOfxStatOK) {
        update();
    }
}

void
CustomParamInteract::focusInEvent(QFocusEvent* /*e*/)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxStatus stat = _imp->entryPoint->gainFocusAction(time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0);
    if (stat == kOfxStatOK) {
        update();
    }
}

void
CustomParamInteract::focusOutEvent(QFocusEvent* /*e*/)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxStatus stat = _imp->entryPoint->loseFocusAction(time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0);
    if (stat == kOfxStatOK) {
        update();
    }
}

void
CustomParamInteract::keyPressEvent(QKeyEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    QByteArray keyStr;
    OfxStatus stat;
    if ( e->isAutoRepeat() ) {
        stat = _imp->entryPoint->keyRepeatAction( time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0, (int)QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), keyStr.data() );
    } else {
        stat = _imp->entryPoint->keyDownAction( time, scale, /*view=*/ 0,_imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0, (int)QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), keyStr.data() );
    }
    if (stat == kOfxStatOK) {
        update();
    }
}

void
CustomParamInteract::keyReleaseEvent(QKeyEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    double time = _imp->knob.lock()->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    QByteArray keyStr;
    OfxStatus stat = _imp->entryPoint->keyUpAction( time, scale, /*view=*/ 0, _imp->entryPoint->hasColorPicker() ? &_imp->entryPoint->getLastColorPickerColor() : /*colourPicker=*/0, (int)QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), keyStr.data() );
    if (stat == kOfxStatOK) {
        update();
    }
}

NATRON_NAMESPACE_EXIT

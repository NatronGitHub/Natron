//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef CUSTOMPARAMINTERACT_H
#define CUSTOMPARAMINTERACT_H

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/GlobalDefines.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Engine/OverlaySupport.h"

class KnobGui;
namespace Natron {
class OfxParamOverlayInteract;
}
struct CustomParamInteractPrivate;
class CustomParamInteract
    : public QGLWidget, public OverlaySupport
{
public:
    CustomParamInteract(KnobGui* knob,
                        void* ofxParamHandle,
                        const boost::shared_ptr<Natron::OfxParamOverlayInteract> & entryPoint,
                        QWidget* parent = 0);

    virtual ~CustomParamInteract();

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;

    /**
     * @brief Repaint
     **/
    virtual void redraw() OVERRIDE FINAL;

    /**
     * @brief Returns the width and height of the viewport in window coordinates.
     **/
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;

    /**
     * @brief Returns the pixel scale of the viewport.
     **/
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;

    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;

    virtual void saveContext() OVERRIDE FINAL;
    virtual void restoreContext() OVERRIDE FINAL;
    
private:

    virtual void paintGL() OVERRIDE FINAL;
    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int w,int h) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<CustomParamInteractPrivate> _imp;
};

#endif // CUSTOMPARAMINTERACT_H

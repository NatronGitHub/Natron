/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef CUSTOMPARAMINTERACT_H
#define CUSTOMPARAMINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/GlobalDefines.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#include <QOpenGLWidget>
#else
#include "Gui/QGLWidgetCompat.h"
#endif

#include "Engine/OverlaySupport.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct CustomParamInteractPrivate;
class CustomParamInteract
    : public QOpenGLWidget, public OverlaySupport
{
public:
    CustomParamInteract(const KnobGuiPtr& knob,
                        void* ofxParamHandle,
                        const OfxParamOverlayInteractPtr & entryPoint,
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

#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL;
#endif
    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;

    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;

private:

    virtual void paintGL() OVERRIDE FINAL;
    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int w, int h) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    std::unique_ptr<CustomParamInteractPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // CUSTOMPARAMINTERACT_H

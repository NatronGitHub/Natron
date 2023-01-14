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

#ifndef Engine_OverlaySupport_h
#define Engine_OverlaySupport_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QPointF>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EngineFwd.h"
#include "Engine/RectD.h"

NATRON_NAMESPACE_ENTER

/**
 * @class An abstract interface for overlay holders. Any OpenGL widget capable of drawing overlays should
 * implement this interface.
 **/

class OverlaySupport
{
public:

    OverlaySupport()
    {
    }

    virtual ~OverlaySupport()
    {
    }

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() = 0;

    /**
     * @brief Repaint
     **/
    virtual void redraw() = 0;

    /**
     * @brief Returns the width and height of the viewport in window coordinates.
     **/
    virtual void getViewportSize(double &width, double &height) const = 0;

    /**
     * @brief Returns the pixel scale of the viewport.
     **/
    virtual void getPixelScale(double & xScale, double & yScale) const  = 0;

#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const = 0;
#endif

    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const = 0;

    /**
     * @brief Must save all relevant OpenGL bits so that they can be restored as-is after the draw action of a plugin.
     **/
    virtual void saveOpenGLContext() = 0;

    /**
     * @brief Must restore all OpenGL bits saved in saveOpenGLContext()
     **/
    virtual void restoreOpenGLContext() = 0;

    /**
     * @brief Get the current mipmapLevel applied by the viewer
     **/
    virtual unsigned int getCurrentRenderScale() const = 0;

    /**
     * @brief Returns whether the given rectangle in canonical coords is visible in the viewport
     **/
    bool isVisibleInViewport(const RectD& rect) const
    {
        RectD visiblePortion = getViewportRect();

        return visiblePortion.intersects(rect);
    }

    /**
     * @brief Returns the viewport visible portion in canonical coordinates
     **/
    virtual RectD getViewportRect() const = 0;

    /**
     * @brief Returns the cursor position in canonical coordinates
     **/
    virtual void getCursorPosition(double& x, double& y) const = 0;

    /**
     * @brief Returns for a viewer the internal viewer node
     **/
    virtual ViewerInstance* getInternalViewerNode() const
    {
        return 0;
    }

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const = 0;
    QPointF toWidgetCoordinates(const QPointF& canonicalCoords) const
    {
        QPointF ret = canonicalCoords;

        toWidgetCoordinates( &ret.rx(), &ret.ry() );

        return ret;
    }

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const = 0;
    QPointF toCanonicalCoordinates(const QPointF& widgetCoords) const
    {
        QPointF ret = widgetCoords;

        toCanonicalCoordinates( &ret.rx(), &ret.ry() );

        return ret;
    }

    /**
     * @brief May be implemented to draw text using the widget's current font
     **/
    virtual bool renderText(double x,
                            double y,
                            const std::string &string,
                            double r,
                            double g,
                            double b,
                            int flags = 0) //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10
    {
        Q_UNUSED(x);
        Q_UNUSED(y);
        Q_UNUSED(string);
        Q_UNUSED(r);
        Q_UNUSED(g);
        Q_UNUSED(b);
        Q_UNUSED(flags);

        return false;
    }

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const = 0;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const = 0;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_OverlaySupport_h

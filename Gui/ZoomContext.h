/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef ZOOMCONTEXT_H
#define ZOOMCONTEXT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#ifndef NDEBUG
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QPointF>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 *@class ZoomContext
 *@brief Holds all zoom related variables. This is an internal class used by the ViewerGL.
 * The variables stored here are the minimal variables needed to enable the zoom and the drag
 * of the image.
 * The top and right edges of the ortographic projection can be computed as such:
 *
 * top = bottom + heightWidget/zoomFactor
 * right = left + widthWidget/zoomFactor
 *
 *
 * During the computations made in the ViewerGL, we define 2 coordinate systems:
 *  - The viewport (or widget) coordinate system, with origin top left.
 *  - The image coordinate system with the origin bottom left.
 * To transform the coordinates between one system to another is a simple mapping operation,
 * which yields :
 *
 * Ximg = (Xwidget/widthWidget) * ( right - left ) + left
 * Yimg = (Ywidget/heightWidget) * (bottom - top) + top  [notice the y inversion here]
 *
 * Let us define the zoomFactor being the ratio of screen pixels size divided by image pixels size.
 *
 * Zooming to a point is simply a matter of changing the orthographic projection.
 * When zooming, the position of the center should never change, relativly to the orthographic projection.
 * Which means that the old position (before zooming) expressed in its own orthographic projection, should equal
 * the new position (after zooming) expressed in its own orthographic projection.
 * That is:
 *
 * - For the x coordinate:  (Ximg - left_old) * zoomFactor_old == (Ximg - left_new) * zoomFactor_new
 * Where Ximg is the X coordinate of the zoom center in image coordinates, left_old is the left edge of
 * the orthographic projection before zooming, and the left_new the left edge after zooming.
 *
 * This formula yields:
 *
 *  left_new = Ximg - (Ximg - left_old)*(zoomFactor_old/zoomFactor_new).
 *
 * -The y coordinate follows exactly the same reasoning and the following equation can be found:
 *
 *  bottom_new = Yimg - (Yimg - bottom_old)*(zoomFactor_old/zoomFactor_new).
 *
 * Retrieving top_new and right_new can be done with the formulas exhibited above.
 *
 * A last note on the zoom is the initialisation. A desired effect can be to initialise the image
 * so it appears centered in the viewer and that fit entirely in the viewer. This can be done as such:
 *
 * The zoomFactor needed to fit the image in the viewer can be computed with the ratio of the
 * height of the widget by the height of the image :
 *
 * zoomFactor = heightWidget / heightImage
 *
 * The left and bottom edges can then be initialised :
 *
 * left = widthImage / 2 - ( widthWidget / ( 2 * zoomFactor ) )
 * bottom = heightImage / 2 - ( heightWidget / ( 2 * zoomFactor ) )
 *
 * TRANSLATING THE IMAGE : (panning around)
 *
 * Translation is simply a matter of displacing the edges of the orthographic projection
 * by a delta. The delta is the difference between the last mouse position (in image coordinates)
 * and the new mouse position (in image coordinates).
 *
 * Translating is just doing so:
 *
 *  bottom += Yimg_old - Yimg_new
 *  left += Ximg_old - Ximg_new
 **/
class ZoomContext
{
public:
    ZoomContext()
        : _zoomLeft(0.)
        , _zoomBottom(0.)
        , _zoomFactor(1.)
        , _zoomAspectRatio(1.)
        , _screenWidth(0)
        , _screenHeight(0)
    {
    }

    /// width of a screen pixel in zoom coordinates
    double screenPixelWidth() const
    {
        assert(_zoomFactor > 0 && _zoomAspectRatio > 0);

        return 1. / (_zoomFactor * _zoomAspectRatio);
    }

    /// height of a screen pixel in zoom coordinates
    double screenPixelHeight() const
    {
        assert(_zoomFactor > 0);

        return 1. / _zoomFactor;
    }

    double left() const
    {
        return _zoomLeft;
    }

    double right() const
    {
        return _zoomLeft + _screenWidth * screenPixelWidth();
    }

    double bottom() const
    {
        return _zoomBottom;
    }

    double top() const
    {
        return _zoomBottom + _screenHeight * screenPixelHeight();
    }

    double factor() const
    {
        return _zoomFactor;
    }

    // the aspect ratio of zoom coordinates. For a Viewer, it should always be 1., since
    // zoom coordinates == canonical coordinates, which are orthonormal
    double aspectRatio() const
    {
        return _zoomAspectRatio;
    }

    /// width in screen pixels of the zoomed area
    double screenWidth() const
    {
        return _screenWidth;
    }

    /// height in screen pixels of the zoomed area
    double screenHeight() const
    {
        return _screenHeight;
    }

    void setZoom(double zoomLeft,
                 double zoomBottom,
                 double zoomFactor,
                 double zoomAspectRatio)
    {
        assert(boost::math::isfinite(zoomLeft) && boost::math::isfinite(zoomBottom) && boost::math::isfinite(zoomFactor) && boost::math::isfinite(zoomAspectRatio) && zoomFactor > 0 && zoomAspectRatio > 0);
        _zoomLeft = zoomLeft;
        _zoomBottom = zoomBottom;
        _zoomFactor = zoomFactor;
        _zoomAspectRatio = zoomAspectRatio;
        check();
    }

    void translate(double dx,
                   double dy)
    {
        assert( boost::math::isfinite(dx) && boost::math::isfinite(dy) );
        _zoomLeft += dx;
        _zoomBottom += dy;
        check();
    }

    void zoom(double centerX,
              double centerY,
              double scale)
    {
        assert(boost::math::isfinite(centerX) && boost::math::isfinite(centerY) && boost::math::isfinite(scale) && scale > 0);
        _zoomLeft = centerX - ( centerX - left() ) / scale;
        _zoomBottom = centerY - ( centerY - bottom() ) / scale;
        _zoomFactor *= scale;
        check();
    }

    // only zoom the x axis: changes the AspectRatio and the Left but not the zoomFactor or the bottom
    void zoomx(double centerX,
               double /*centerY*/,
               double scale)
    {
        assert(boost::math::isfinite(centerX) && /*boost::math::isfinite(centerY) &&*/ boost::math::isfinite(scale) && scale > 0);
        _zoomLeft = centerX - ( centerX - left() ) / scale;
        _zoomAspectRatio *= scale;
        check();
    }

    // only zoom the y axis: changes the AspectRatio, the zoomFactor and the Bottom but not the Left
    void zoomy(double /*centerX*/,
               double centerY,
               double scale)
    {
        assert(/*boost::math::isfinite(centerX) &&*/ boost::math::isfinite(centerY) && boost::math::isfinite(scale) && scale > 0);
        _zoomBottom = centerY - ( centerY - bottom() ) / scale;
        _zoomAspectRatio /= scale;
        _zoomFactor *= scale;
        check();
    }

    // fit the area (xmin-xmax,ymin-ymax) in the zoom window, without modifying the AspectRatio
    void fit(double xmin,
             double xmax,
             double ymin,
             double ymax)
    {
        assert(boost::math::isfinite(xmin) && boost::math::isfinite(xmax) && boost::math::isfinite(ymin) && boost::math::isfinite(ymax) && xmin < xmax && ymin < ymax);
        double width = xmax - xmin;
        double height = ymax - ymin;

        if ( screenWidth() / ( screenHeight() * aspectRatio() ) < (width / height) ) {
            _zoomLeft = xmin;
            _zoomFactor = screenWidth() / ( width * aspectRatio() );
            _zoomBottom = (ymax + ymin) / 2. - ( ( screenHeight() * aspectRatio() ) / screenWidth() ) * width / 2.;
        } else {
            _zoomBottom = ymin;
            _zoomFactor = screenHeight() / height;
            _zoomLeft = (xmax + xmin) / 2. - ( screenWidth() / ( screenHeight() * aspectRatio() ) ) * height / 2.;
        }
        check();
    }

    // fill the area (xmin-xmax,ymin-ymax) in the zoom window, modifying the AspectRatio
    void fill(double xmin,
              double xmax,
              double ymin,
              double ymax)
    {
        assert(boost::math::isfinite(xmin) && boost::math::isfinite(xmax) && boost::math::isfinite(ymin) && boost::math::isfinite(ymax) && xmin < xmax && ymin < ymax);
        double width = xmax - xmin;
        double height = ymax - ymin;

        _zoomLeft = xmin;
        _zoomBottom = ymin;
        _zoomFactor = screenHeight() / height;
        _zoomAspectRatio = (screenWidth() * height) / (screenHeight() * width);
        check();
    }

    void setScreenSize(double screenWidth,
                       double screenHeight,
                       bool alignTop = false,
                       bool alignRight = false)
    {
        assert(boost::math::isfinite(screenWidth) && boost::math::isfinite(screenHeight) && screenWidth > 0 && screenHeight > 0);
        if ( (screenWidth <= 0) || (screenHeight <= 0) ) {
            _screenWidth = 0;
            _screenHeight = 0;
            check();

            return;
        }
        assert(screenWidth > 0 && screenHeight > 0);
        if (alignTop) {
            // old top: _zoomBottom + _screenHeight / _zoomFactor
            // new top: _zoomBottom + screenHeight / _zoomFactor
            _zoomBottom -= (screenHeight - _screenHeight) / _zoomFactor;
        }
        if (alignRight) {
            // old right: _zoomLeft + _screenWidth / (_zoomFactor * _zoomAspectRatio)
            // new right: _zoomLeft + screenWidth / (_zoomFactor * _zoomAspectRatio)
            _zoomLeft -= (screenWidth - _screenWidth) / (_zoomFactor * _zoomAspectRatio);
        }
        _screenWidth = screenWidth;
        _screenHeight = screenHeight;
        check();
    }

    /**
     *@brief Computes the image coordinates of the point passed in parameter.
     * This is a fast in-line method much faster than toZoomCoordinates_slow().
     * This function actually does the unprojection to retrieve the position.
     *@param x[in] The x coordinate of the point in viewport coordinates.
     *@param y[in] The y coordinates of the point in viewport coordinates.
     *@returns Returns the image coordinates mapped equivalent of (x,y).
     **/
    QPointF toZoomCoordinates(double widgetX,
                              double widgetY) const
    {
        assert(boost::math::isfinite(widgetX) && boost::math::isfinite(widgetY) && _screenWidth > 0 && _screenHeight > 0);
        if ( (_screenWidth <= 0) || (_screenHeight <= 0) ) {
            return QPointF( left(), top() );
        }

        return QPointF( ( ( ( right() - left() ) * widgetX ) / screenWidth() ) + left(),
                        ( ( ( bottom() - top() ) * widgetY ) / screenHeight() ) + top() );
    }

    /**
     *@brief Computes the viewport coordinates of the point passed in parameter.
     * This function actually does the projection to retrieve the position;
     *@param x[in] The x coordinate of the point in image coordinates.
     *@param y[in] The y coordinates of the point in image coordinates.
     *@returns Returns the viewport coordinates mapped equivalent of (x,y).
     **/
    QPointF toWidgetCoordinates(double zoomX,
                                double zoomY) const
    {
        assert( boost::math::isfinite(zoomX) && boost::math::isfinite(zoomY) );

        return QPointF( ( ( zoomX - left() ) / ( right() - left() ) ) * screenWidth(),
                        ( ( zoomY - top() ) / ( bottom() - top() ) ) * screenHeight() );
    }

private:
    void check()
    {
        assert( boost::math::isfinite(_zoomLeft) && boost::math::isfinite(_zoomBottom) && boost::math::isfinite(_zoomFactor) && boost::math::isfinite(_zoomAspectRatio) && boost::math::isfinite(_screenWidth) && boost::math::isfinite(_screenHeight) );
    }

private:
    double _zoomLeft; /// the left edge of the orthographic projection
    double _zoomBottom; /// the bottom edge of orthographic projection
    double _zoomFactor; /// the zoom factor applied to the current image
    double _zoomAspectRatio; /// the aspect ratio; a 1x1 area in zoom coordinates occupies a size (zoomFactor*zoomPar,zoomFactor)
    double _screenWidth; /// window width in screen pixels
    double _screenHeight; /// window height in screen pixels
};

NATRON_NAMESPACE_EXIT

#endif // ZOOMCONTEXT_H

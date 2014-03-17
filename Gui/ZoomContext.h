//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef ZOOMCONTEXT_H
#define ZOOMCONTEXT_H

#include <QPointF>

/**
 *@class ZoomContext
 *@brief Holds all zoom related variables. This is an internal class used by the ViewerGL.
 *The variables stored here are the minimal variables needed to enable the zoom and the drag
 *of the image.
 *The top and right edges of the ortographic projection can be computed as such:
 *
 * top = bottom + heightWidget/zoomFactor
 * right = left + widthWidget/zoomFactor
 *
 *
 *During the computations made in the ViewerGL, we define 2 coordinate systems:
 *  - The viewport (or widget) coordinate system, with origin top left.
 *  - The image coordinate system with the origin bottom left.
 *To transform the coordinates between one system to another is a simple mapping operation,
 *which yields :
 *
 * Ximg = (Xwidget/widthWidget) * ( right - left ) + left
 * Yimg = (Ywidget/heightWidget) * (bottom - top) + top  [notice the y inversion here]
 *
 *Let us define the zoomFactor being the ratio of screen pixels size divided by image pixels size.
 *
 *Zooming to a point is simply a matter of changing the orthographic projection.
 *When zooming, the position of the center should never change, relativly to the orthographic projection.
 *Which means that the old position (before zooming) expressed in its own orthographic projection, should equal
 *the new position (after zooming) expressed in its own orthographic projection.
 *That is:
 *
 * - For the x coordinate:  (Ximg - left_old) * zoomFactor_old == (Ximg - left_new) * zoomFactor_new
 *Where Ximg is the X coordinate of the zoom center in image coordinates, left_old is the left edge of
 *the orthographic projection before zooming, and the left_new the left edge after zooming.
 *
 *This formula yields:
 *
 *  left_new = Ximg - (Ximg - left_old)*(zoomFactor_old/zoomFactor_new).
 *
 * -The y coordinate follows exactly the same reasoning and the following equation can be found:
 *
 *  bottom_new = Yimg - (Yimg - bottom_old)*(zoomFactor_old/zoomFactor_new).
 *
 * Retrieving top_new and right_new can be done with the formulas exhibited above.
 *
 *A last note on the zoom is the initialisation. A desired effect can be to initialise the image
 *so it appears centered in the viewer and that fit entirely in the viewer. This can be done as such:
 *
 *The zoomFactor needed to fit the image in the viewer can be computed with the ratio of the
 *height of the widget by the height of the image :
 *
 * zoomFactor = heightWidget / heightImage
 *
 *The left and bottom edges can then be initialised :
 *
 * left = widthImage / 2 - ( widthWidget / ( 2 * zoomFactor ) )
 * bottom = heightImage / 2 - ( heightWidget / ( 2 * zoomFactor ) )
 *
 *TRANSLATING THE IMAGE : (panning around)
 *
 *Translation is simply a matter of displacing the edges of the orthographic projection
 *by a delta. The delta is the difference between the last mouse position (in image coordinates)
 *and the new mouse position (in image coordinates).
 *
 *Translating is just doing so:
 *
 *  bottom += Yimg_old - Yimg_new
 *  left += Ximg_old - Ximg_new
 **/
class ZoomContext {
public:
    ZoomContext()
    : _zoomLeft(0.)
    , _zoomBottom(0.)
    , _zoomFactor(1.)
    , _zoomPAR(1.)
    , _screenWidth(0)
    , _screenHeight(0)
    {}

    /// width of a screen pixel in zoom coordinates
    double screenPixelWidth() const
    {
        return 1./ (_zoomFactor * _zoomPAR);
    }

    /// height of a screen pixel in zoom coordinates
    double screenPixelHeight() const
    {
        return 1./ _zoomFactor;
    }

    double left() const
    {
        return _zoomLeft;
    }

    double right() const
    {
        return _zoomLeft + _screenWidth / (_zoomFactor * _zoomPAR);
    }

    double bottom() const
    {
        return _zoomBottom;
    }

    double top() const
    {
        return _zoomBottom + _screenHeight / _zoomFactor;
    }

    double factor() const
    {
        return _zoomFactor;
    }

    double par() const
    {
        return _zoomPAR;
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

    void setZoom(double zoomLeft, double zoomBottom, double zoomFactor, double zoomPAR)
    {
        _zoomLeft = zoomLeft;
        _zoomBottom = zoomBottom;
        _zoomFactor = zoomFactor;
        _zoomPAR = zoomPAR;
    }

    void translate(double dx, double dy)
    {
        _zoomLeft += dx;
        _zoomBottom += dy;
    }

    void zoom(double centerX, double centerY, double scale)
    {
        _zoomLeft = centerX - (centerX - left()) / scale;
        _zoomBottom = centerY - (centerY - bottom()) / scale;
        _zoomFactor *= scale;
    }

    // only zoom the x axis: changes the PAR and the Left but not the zoomFactor or the bottom
    void zoomx(double centerX, double centerY, double scale)
    {
        _zoomLeft = centerX - (centerX - left()) / scale;
        _zoomPAR *= scale;
    }

    // only zoom the y axis: changes the PAR, the zoomFactor and the Bottom but not the Left
    void zoomy(double centerX, double centerY, double scale)
    {
        _zoomBottom = centerY - (centerY - bottom()) / scale;
        _zoomPAR /= scale;
        _zoomFactor *= scale;
    }

    // fit the area (xmin-xmax,ymin-ymax) in the zoom window, without modifying the PAR
    void fit(double xmin,double xmax,double ymin,double ymax)
    {
        double width = xmax - xmin;
        double height = ymax - ymin;
        if (screenWidth() / (screenHeight() * par()) < (width / height)) {
            _zoomLeft = xmin;
            _zoomFactor = screenWidth() / (width * par());
            _zoomBottom = (ymax + ymin) / 2. - ((screenHeight() * par())/ screenWidth()) * width / 2.;
        } else {
            _zoomBottom = ymin;
            _zoomFactor = screenHeight() / height;
            _zoomLeft = (xmax + xmin) / 2. - (screenWidth() / (screenHeight() * par())) * height / 2.;
        }
    }

    // fill the area (xmin-xmax,ymin-ymax) in the zoom window, modifying the PAR
    void fill(double xmin,double xmax,double ymin,double ymax)
    {
        double width = xmax - xmin;
        double height = ymax - ymin;
        _zoomLeft = xmin;
        _zoomBottom = ymin;
        _zoomFactor = screenHeight() / height;
        _zoomPAR = (screenWidth() * height) / (screenHeight() * width);
    }

    void setScreenSize(double screenWidth, double screenHeight)
    {
        _screenWidth = screenWidth;
        _screenHeight = screenHeight;
    }

    /**
     *@brief Computes the image coordinates of the point passed in parameter.
     *This is a fast in-line method much faster than toZoomCoordinates_slow().
     *This function actually does the unprojection to retrieve the position.
     *@param x[in] The x coordinate of the point in viewport coordinates.
     *@param y[in] The y coordinates of the point in viewport coordinates.
     *@returns Returns the image coordinates mapped equivalent of (x,y).
     **/
    QPointF toZoomCoordinates(double widgetX, double widgetY)
    {
        return QPointF((((right() - left())*widgetX)/screenWidth())+left(),
                       (((bottom() - top())*widgetY)/screenHeight())+top());
    }

    /**
     *@brief Computes the viewport coordinates of the point passed in parameter.
     *This function actually does the projection to retrieve the position;
     *@param x[in] The x coordinate of the point in image coordinates.
     *@param y[in] The y coordinates of the point in image coordinates.
     *@returns Returns the viewport coordinates mapped equivalent of (x,y).
     **/
    QPointF toWidgetCoordinates(double zoomX, double zoomY)
    {
        return QPointF(((zoomX - left())/(right() - left()))*screenWidth(),
                      ((zoomY - top())/(bottom() - top()))*screenHeight());
    }

private:
    double _zoomLeft; /// the left edge of the orthographic projection
    double _zoomBottom; /// the bottom edge of orthographic projection
    double _zoomFactor; /// the zoom factor applied to the current image
    double _zoomPAR; /// the pixel aspect ration; a pixel from the image data occupies a size (zoomFactor*zoomPar,zoomFactor)
    double _screenWidth; /// window width in screen pixels
    double _screenHeight; /// window height in screen pixels
};


#endif // ZOOMCONTEXT_H

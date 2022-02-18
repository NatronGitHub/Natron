/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef OPENGLVIEWERI_H
#define OPENGLVIEWERI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include "Engine/OverlaySupport.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/Texture.h"


NATRON_NAMESPACE_ENTER


class OpenGLViewerI
    : public OverlaySupport
{
public:


    OpenGLViewerI()
    {
    }

    virtual ~OpenGLViewerI()
    {
    }

    virtual bool isViewerUIVisible() const = 0;

    /**
     * @brief Must return the current scale factor applied to the viewport. (Screen pixels / image pixels)
     **/
    virtual double getZoomFactor() const = 0;

    /**
     * @brief Must center the image in the viewport so it fits entirely.
     **/
    virtual void fitImageToFormat() = 0;

    /**
     * @brief Must return true if the portion of the image displayed is clipped to the input format
     **/
    virtual bool isClippingImageToFormat() const = 0;

    /**
     * @brief Given the region of definition of an image, must return the portion of that image which is
     * actually displayed on the viewport. (It cannot be bigger than the rod)
     **/
    virtual RectI getImageRectangleDisplayed(const RectI & pixelRod, const double par, unsigned int mipMapLevel) = 0;
    virtual RectI getImageRectangleDisplayedRoundedToTileSize(int texIndex, const RectD & rod, const double par, unsigned int mipMapLevel,
                                                              std::vector<RectI>* tiles, std::vector<RectI>* tilesRounded, int *tileSize, RectI* roiNotRounded) = 0;
    virtual RectI getExactImageRectangleDisplayed(int texIndex, const RectD & rod, const double par, unsigned int mipMapLevel) = 0;

    /**
     * @brief Must return the bit depth of the texture used to render. (Byte, half or float)
     **/
    virtual ImageBitDepthEnum getBitDepth() const = 0;

    /**
     * @brief Returns true if the user has enabled the region of interest
     **/
    virtual bool isUserRegionOfInterestEnabled() const = 0;

    /**
     * @brief Must return the user's region of interest rectangle in canonical coordinates.
     **/
    virtual RectD getUserRegionOfInterest() const = 0;

    /**
     * @brief Should clear any partial texture overlayed previously transferred with transferBufferFromRAMtoGPU
     **/
    virtual void clearPartialUpdateTextures()  = 0;

    /**
     * @brief This function must do the following:
     * 1) glMapBuffer to map a GPU buffer to the RAM
     * 2) memcpy to copy the ramBuffer to previously mapped buffer.
     * 3) glUnmapBuffer to unmap the GPU buffer
     * 4) glTexSubImage2D or glTexImage2D depending whether yo need to resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const unsigned char* ramBuffer,
                                            size_t bytesCount,
                                            const RectI &roiRoundedToTileSize,
                                            const RectI& roi,
                                            const TextureRect & tileRect,
                                            int textureIndex,
                                            bool isPartialRect,
                                            bool isFirstTile,
                                            TexturePtr* texture) = 0;
    virtual void endTransferBufferFromRAMToGPU(int textureIndex,
                                               const TexturePtr& texture,
                                               const ImagePtr& image,
                                               int time,
                                               const RectD& rod,
                                               double par,
                                               ImageBitDepthEnum depth,
                                               unsigned int mipMapLevel,
                                               ImagePremultiplicationEnum premult,
                                               double gain,
                                               double gamma,
                                               double offset,
                                               int lut,
                                               bool recenterViewer,
                                               const Point& viewportCenter,
                                               bool isPartialRect) = 0;

    /**
     * @brief Called when the input of a viewer should render black.
     **/
    virtual void disconnectInputTexture(int textureIndex, bool clearRoD) = 0;

    /**
     * @brief This function should update the color picker values (as a label or numbers) right away.
     * If x and y are INT_MAX then the viewer should just use the current position of the cursor.
     **/
    virtual void updateColorPicker(int textureIndex, int x = INT_MAX, int y = INT_MAX) = 0;

    /**
     * @brief Must use OpenGL to query the texture color at the given image coordinates
     * X and Y are in canonical coordinates
     **/
    virtual void getTextureColorAt(int x, int y, double* r, double *g, double *b, double *a) = 0;

    /**
     * @brief Make the OpenGL context current to the thread.
     **/
    virtual void makeOpenGLcontextCurrent()  = 0;


    /**
     * @brief Called when the live instance of the viewer node is killed. (i.e: when the node is deleted).
     * You should remove any GUI.
     **/
    virtual void removeGUI() = 0;

    /**
     * @brief Must return the current view displayed if multi-view is enabled, 0 otherwise.
     **/
    virtual ViewIdx getCurrentView() const = 0;

    /**
     * @brief Must return the time currently displayed
     **/
    virtual int getCurrentlyDisplayedTime() const = 0;

    /**
     * @brief Get the viewer's timeline's range
     **/
    virtual void getViewerFrameRange(int* first, int* last) const = 0;

    /**
     * @brief Must return the current compositing operator applied to the viewer input A and B.
     **/
    virtual ViewerCompositingOperatorEnum getCompositingOperator() const = 0;

    /**
     * @brief Set the current compositing operator
     **/
    virtual void setCompositingOperator(ViewerCompositingOperatorEnum op) = 0;

    /**
     * @brief Must return a pointer to the current timeline used by the Viewer
     **/
    virtual TimeLinePtr getTimeline() const = 0;

    /**
     * @brief Must save all relevant OpenGL bits so that they can be restored as-is after the draw action of a plugin.
     **/
    virtual void saveOpenGLContext() OVERRIDE = 0;

    /**
     * @brief Must restore all OpenGL bits saved in saveOpenGLContext()
     **/
    virtual void restoreOpenGLContext() OVERRIDE = 0;

    /**
     * @brief Clears pointers to images that may be left
     **/
    virtual void clearLastRenderedImage() = 0;

    /**
     *@brief To be called if redraw needs to  be called now without waiting the end of the event loop
     **/
    virtual void redrawNow() = 0;

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE = 0;

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE = 0;

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE = 0;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE = 0;
};

NATRON_NAMESPACE_EXIT

#endif // OPENGLVIEWERI_H

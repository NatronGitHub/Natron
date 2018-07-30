/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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
#include "Engine/Texture.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

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
     * @brief Given the region of definition of an image, must return the portion of that image which is
     * actually displayed on the viewport. (It cannot be bigger than the rod)
     **/
    virtual RectD getImageRectangleDisplayed() const = 0;
    /**
     * @brief Should clear any partial texture overlayed previously transferred with transferBufferFromRAMtoGPU
     **/
    virtual void clearPartialUpdateTextures()  = 0;

    struct TextureTransferArgs
    {
        enum TypeEnum
        {
            // The newly created texture will replace the existing one in the given viewer input
            eTextureTransferTypeReplace,

            // The newly created texture will be drawn as an overlay over the current texture.
            // This is used for example during tracking to only update small areas of the viewer
            eTextureTransferTypeOverlay,

            // If no texture already exists or it does not contain the image bounds
            // this mode is similar to eTextureTransferTypeReplace.
            // Otherwise only the portion corresponding to the image bounds is updated in the current texture.
            // This mode is used while drawing a preview with the RotoPaint node.
            eTextureTransferTypeModify
        };

        // What should the viewer do with the provided image, see the enum
        TypeEnum type;

        // 0 or 1: Indicates if we want to upload to the A(=0) viewer input or the B (=1) viewer input
        int textureIndex;

        // The image to upload as a texture: this must be a RAM image
        ImagePtr image;

        // The color picker image: this is the image produced by the node upstream of the
        // ViewerProcess node corresponding to the given textureIndex
        ImagePtr colorPickerImage;

        // The color picker image input: this is the image produced by the input node of the node
        // upstrea m of the ViewerProcess node corresponding to the given textureIndex
        ImagePtr colorPickerInputImage;

        // This is the time at which the image was rendered.
        TimeValue time;

        // This is the view at which the image was rendered
        ViewIdx view;

        // This is the region of definition that was used to produce this image
        RectD rod;

        // If true, the viewport will center its projection on the viewportCenter point
        bool recenterViewer;

        // If recenterViewer is true, the viewport will center its projection on this point
        Point viewportCenter;

        // This is the image cache key of the image produced by the viewer process node at the bottom
        // of the tree. This is used in turn by the timeline to update the cached frames line.
        ImageCacheKeyPtr viewerProcessNodeKey;

        TextureTransferArgs()
        : type(eTextureTransferTypeReplace)
        , textureIndex(0)
        , image()
        , colorPickerImage()
        , colorPickerInputImage()
        , time(0)
        , view(0)
        , rod()
        , recenterViewer(false)
        , viewportCenter()
        , viewerProcessNodeKey()
        {

        }

    };

    /**
     * @brief This function must do the following:
     * 1) glMapBuffer to map a GPU buffer to the RAM
     * 2) memcpy to copy the ramBuffer to previously mapped buffer.
     * 3) glUnmapBuffer to unmap the GPU buffer
     * 4) glTexSubImage2D or glTexImage2D depending whether yo need to resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const TextureTransferArgs& args) = 0;

    /**
     * @brief Clear the image pointers of the last image sent to transferBufferFromRAMtoGPU
     **/
    virtual void clearLastRenderedImage() = 0;

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
     * @brief Called when the viewer should refresh the foramt
     **/
    virtual void refreshMetadata(int inputNb, const NodeMetadata& metadata) = 0;

    /**
     * @brief Called when the live instance of the viewer node is killed. (i.e: when the node is deleted).
     * You should remove any GUI.
     **/
    virtual void removeGUI() = 0;

    /**
     * @brief Must return the time currently displayed
     **/
    virtual TimeValue getCurrentlyDisplayedTime() const = 0;

    /**
     * @brief Get the viewer's timeline's range
     **/
    virtual void getViewerFrameRange(int* first, int* last) const = 0;

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

    /**
     * @brief Get the current orthographic projection
     **/
    virtual void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const = 0;

    /**
     * @brief Set the current orthographic projection
     **/
    virtual void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio) = 0;

    /**
     * @brief Translates the viewport by dx, dy
     **/
    virtual void translateViewport(double dx, double dy) = 0;

    /**
     * @brief Zoom the viewport so that it's new scale factor is equal to newZoomFactor
     **/
    virtual void zoomViewport(double newZoomFactor) = 0;

    /**
     * @brief Set the color picker info bar visibility
     **/
    virtual void setInfoBarVisible(bool visible) = 0;

    /**
     * @brief Set the color picker info bar visibility for one input only
     **/
    virtual void setInfoBarVisible(int index, bool visible) = 0;

    /**
     * @brief Set the left toolbar visibility (the one used by Roto)
     **/
    virtual void setLeftToolBarVisible(bool visible) = 0;

    /**
     * @brief Set the left toolbar visibility
     **/
    virtual void setTopToolBarVisible(bool visible) = 0;

    /**
     * @brief Set the left toolbar visibility
     **/
    virtual void setPlayerVisible(bool visible) = 0;

    /**
     * @brief Set the left toolbar visibility
     **/
    virtual void setTimelineVisible(bool visible) = 0;

    /**
     * @brief Set the tab header visible
     **/
    virtual void setTabHeaderVisible(bool visible) = 0;

    /**
     * @brief Set the timeline bounds on the GUI
     **/
    virtual void setTimelineBounds(double first, double last) = 0;

    /**
     * @brief Set the timeline to display frames or timecode
     **/
    virtual void setTimelineFormatFrames(bool value) = 0;

    /**
     * @brief Set triple sync enabled
     **/
    virtual void setTripleSyncEnabled(bool toggled) = 0;

    virtual OSGLContextPtr getOpenGLViewerContext() const = 0;
};

NATRON_NAMESPACE_EXIT

#endif // OPENGLVIEWERI_H

//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef OPENGLVIEWERI_H
#define OPENGLVIEWERI_H

#include <boost/shared_ptr.hpp>
#include "Engine/OverlaySupport.h"
#include "Engine/Rect.h"
class Format;
struct TextureRect;
class QString;
class TimeLine;
namespace Natron
{
    class Image;
}

class OpenGLViewerI
    : public OverlaySupport
{
public:

    enum BitDepth
    {
        BYTE = 0,
        HALF_FLOAT,
        FLOAT
    };

    OpenGLViewerI()
    {
    }

    virtual ~OpenGLViewerI()
    {
    }

    /**
     * @brief Must return the current scale factor applied to the viewport. (Screen pixels / image pixels)
     **/
    virtual double getZoomFactor() const = 0;

    /**
     * @brief Must center the image in the viewport so it fits entirely.
     **/
    virtual void fitImageToFormat() = 0;

    /**
     * @brief Must return true if the portion of the image displayed is clipped to the project window.
     **/
    virtual bool isClippingImageToProjectWindow() const = 0;

    /**
     * @brief Given the region of definition of an image, must return the portion of that image which is
     * actually displayed on the viewport. (It cannot be bigger than the rod)
     **/
    virtual RectI getImageRectangleDisplayed(const RectI & rod,const double par,unsigned int mipMapLevel) = 0;

    /**
     * @brief Must return the bit depth of the texture used to render. (Byte, half or float)
     **/
    virtual BitDepth getBitDepth() const = 0;

    /**
     * @brief Returns true if the user has enabled the region of interest
     **/
    virtual bool isUserRegionOfInterestEnabled() const = 0;

    /**
     * @brief Must return the user's region of interest rectangle in canonical coordinates.
     **/
    virtual RectD getUserRegionOfInterest() const = 0;

    /**
     * @brief This function must do the following:
     * 1) glMapBuffer to map a GPU buffer to the RAM
     * 2) memcpy to copy the ramBuffer to previously mapped buffer.
     * 3) glUnmapBuffer to unmap the GPU buffer
     * 4) glTexSubImage2D or glTexImage2D depending whether yo need to resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const unsigned char* ramBuffer,
                                            const boost::shared_ptr<Natron::Image>& image,
                                            const RectD& rod,
                                            size_t bytesCount,
                                            const TextureRect & region,
                                            double gain, double offset, int lut,
                                            int pboIndex,
                                            unsigned int mipMapLevel,
                                            Natron::ImagePremultiplication premult,
                                            int textureIndex) = 0;

    /**
     * @brief Called when the input of a viewer should render black.
     **/
    virtual void disconnectInputTexture(int textureIndex) = 0;

    /**
     * @brief This function should update the color picker values (as a label or numbers) right away.
     * If x and y are INT_MAX then the viewer should just use the current position of the cursor.
     **/
    virtual void updateColorPicker(int textureIndex,int x = INT_MAX,int y = INT_MAX) = 0;

    /**
     * @brief Must use OpenGL to query the texture color at the given image coordinates
     * X and Y are in canonical coordinates
     **/
    virtual void getTextureColorAt(int x,int y,double* r,double *g,double *b,double *a) = 0;

    /**
     * @brief Make the OpenGL context current to the thread.
     **/
    virtual void makeOpenGLcontextCurrent()  = 0;

    /**
     * @brief Must return true if the current platform supports GLSL.
     **/
    virtual bool supportsGLSL() const = 0;

    /**
     * @brief Overrides to refresh any gui indicating the name of the underlying node.
     **/
    virtual void onViewerNodeNameChanged(const QString & name) = 0;

    /**
     * @brief Called when the live instance of the viewer node is killed. (i.e: when the node is deleted).
     * You should remove any GUI.
     **/
    virtual void removeGUI() = 0;

    /**
     * @brief Must return the current view displayed if multi-view is enabled, 0 otherwise.
     **/
    virtual int getCurrentView() const = 0;

    /**
     * @brief Must return the current compositing operator applied to the viewer input A and B.
     **/
    virtual Natron::ViewerCompositingOperator getCompositingOperator() const = 0;

    /**
     * @brief Returns whether the user sould be able to edit the frame range or not.
     **/
    virtual bool isFrameRangeLocked() const = 0;
    
    /**
     * @brief Must return a pointer to the current timeline used by the Viewer
     **/
    virtual boost::shared_ptr<TimeLine> getTimeline() const = 0;
    
    /**
     * @brief Must save all relevant OpenGL bits so that they can be restored as-is after the draw action of a plugin.
     **/
    virtual void saveOpenGLContext() = 0;
    
    /**
     * @brief Must restore all OpenGL bits saved in saveOpenGLContext()
     **/
    virtual void restoreOpenGLContext() = 0;
};


#endif // OPENGLVIEWERI_H

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_VIEWERGL_H
#define NATRON_GUI_VIEWERGL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <utility>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/OpenGLViewerI.h"
#include "Global/Macros.h"

class QKeyEvent;
class QEvent;
class QMenu;
class QGLShaderProgram;

namespace Natron {
class ChannelSet;
class Image;
}
class InfoViewerWidget;
class AppInstance;
class ViewerInstance;
class ViewerTab;
class ImageInfo;
class QInputEvent;
struct TextureRect;
class Format;

/**
 *@class ViewerGL
 *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
 * OpenGL related code as well as user events.
 **/
class ViewerGL
    : public QGLWidget, public OpenGLViewerI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    /*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting.
       When constructing a viewer for the 1st time in the app, you must pass a NULL shareWidget. Otherwise,you
       can pass a pointer to the 1st viewer you created. It allows the viewers to share the same OpenGL context.
     */
    explicit ViewerGL(ViewerTab* parent,
                      const QGLWidget* shareWidget = NULL);


    virtual ~ViewerGL() OVERRIDE;

    virtual QSize sizeHint() const OVERRIDE FINAL;
    const QFont & textFont() const;

    void setTextFont(const QFont & f);


    /**
     *@returns Returns true if the viewer is displaying something.
     **/
    bool displayingImage() const;


    /**
     *@returns Returns a const reference to the dataWindow of the currentFrame(BBOX) in canonical coordinates
     **/
    // MT-SAFE: used only by the main-thread, can return a const ref
    const RectD& getRoD(int textureIndex) const;


    /**
     *@returns Returns the displayWindow of the currentFrame(Resolution)
     **/
    // MT-SAFE: don't return a reference
    Format getDisplayWindow() const;


    void setClipToDisplayWindow(bool b);

    virtual bool isClippingImageToProjectWindow() const OVERRIDE FINAL;


    virtual Natron::ImageBitDepthEnum getBitDepth() const OVERRIDE FINAL;

    /**
     *@brief Hack to allow the resizeEvent to be publicly used elsewhere.
     * It calls QGLWidget::resizeEvent(QResizeEvent*).
     **/
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

    /**
     *@returns Returns the height of the frame with the scale factor applied to it.
     **/
    //double zoomedHeight(){return std::floor(getDisplayWindow().height()*_zoomCtx._zoomFactor);}

    /**
     *@returns Returns the width of the frame with the scale factor applied to it.
     **/
    //double zoomedWidth(){return std::floor(getDisplayWindow().width()*_zoomCtx._zoomFactor);}

    /**
     *@returns Returns the current zoom factor that is applied to the display.
     **/
    double getZoomFactor() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns the rectangle of the image displayed by the viewer
     **/
    virtual RectI getImageRectangleDisplayed(const RectI & imageRoD,const double par,unsigned int mipMapLevel) OVERRIDE FINAL;
    virtual RectI getExactImageRectangleDisplayed(const RectD & rod,const double par,unsigned int mipMapLevel) OVERRIDE FINAL;
    virtual RectI getImageRectangleDisplayedRoundedToTileSize(const RectD & rod,const double par,unsigned int mipMapLevel) OVERRIDE FINAL WARN_UNUSED_RETURN;
    /**
     *@brief Set the pointer to the InfoViewerWidget. This is called once after creation
     * of the ViewerGL.
     **/
    void setInfoViewer(InfoViewerWidget* i,int textureIndex);

    /**
     *@brief Handy function that zoom automatically the viewer so it fit
     * the displayWindow  entirely in the viewer
     **/
    virtual void fitImageToFormat() OVERRIDE FINAL;

    void fitImageToFormat(bool useProjectFormat);
    
    /**
     *@brief Turns on the overlays on the viewer.
     **/
    void turnOnOverlay();

    /**
     *@brief Turns off the overlays on the viewer.
     **/
    void turnOffOverlay();


    /**
     *@brief Copies the data stored in the  RAM buffer into the currently
     * used texture.
     * It does:
     * 1) glMapBuffer
     * 2) memcpy to copy data from RAM to GPU
     * 3) glUnmapBuffer
     * 4) glTexSubImage2D or glTexImage2D depending whether we resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const unsigned char* ramBuffer,
                                            const std::list<boost::shared_ptr<Natron::Image> >& tiles,
                                            Natron::ImageBitDepthEnum depth,
                                            int time,
                                            const RectD& rod,
                                            size_t bytesCount, const TextureRect & region,
                                            double gain, double gamma, double offset, int lut, int pboIndex,
                                            unsigned int mipMapLevel,Natron::ImagePremultiplicationEnum premult,
                                            int textureIndex,
                                            const RectI& roi,
                                            bool updateOnlyRoi) OVERRIDE FINAL;
    
    virtual void clearLastRenderedImage() OVERRIDE FINAL;
    
    virtual void disconnectInputTexture(int textureIndex) OVERRIDE FINAL;
    /**
     *@returns Returns true if the graphic card supports GLSL.
     **/
    virtual bool supportsGLSL() const OVERRIDE FINAL;

    /**
     *@brief Disconnects the viewer.
     * Clears out the viewer and reset the viewer info. Note that calling this
     * function while the engine is processing will abort the engine.
     **/
    void disconnectViewer();

    void updatePersistentMessage();
    void updatePersistentMessageToWidth(int w);
    

    virtual void getViewerFrameRange(int* first,int* last) const OVERRIDE FINAL;
    
public Q_SLOTS:


    /**
     *@brief Slot used by the GUI to zoom at the current center the viewport.
     *@param v[in] an integer holding the desired zoom factor (in percent).
     **/
    void zoomSlot(int v);

    /**
     *@brief Convenience function. See ViewerGL::zoomSlot(int)
     **/
    void zoomSlot(double v)
    {
        zoomSlot( (int)v );
    }


    void setRegionOfDefinition(const RectD & rod, double par, int textureIndex);

    virtual void updateColorPicker(int textureIndex,int x = INT_MAX,int y = INT_MAX) OVERRIDE FINAL;

    void clearColorBuffer(double r = 0.,double g = 0.,double b = 0.,double a = 1.);

    void toggleOverlays();
    
    void toggleWipe();
    
    void centerWipe();

    void onProjectFormatChanged(const Format & format);
    
    void onCheckerboardSettingsChanged();

    
    /**
     * @brief Reset the wipe position so it is in the center of the B input.
     * If B input is disconnected it goes in the middle of the A input.
     * Otherwise it goes in the middle of the project window
     **/
    void resetWipeControls();
    
    void clearLastRenderedTexture();
    
private:
    
    void onProjectFormatChangedInternal(const Format & format,bool triggerRender);
public:
    

    virtual void makeOpenGLcontextCurrent() OVERRIDE FINAL;
    virtual void removeGUI() OVERRIDE FINAL;
    virtual int getCurrentView() const OVERRIDE FINAL;
    
    virtual boost::shared_ptr<TimeLine> getTimeline() const OVERRIDE FINAL;


public:

    void renderText(double x, double y, const QString &string, const QColor & color, const QFont & font);

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const;

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);
    
    /**
     * @brief Returns whether the given rectangle is visible in the viewport, in zoom (OpenGL) coordinates.
     **/
    bool isVisibleInViewport(const RectD& rectangle) const;

    void setUserRoIEnabled(bool b);
    
    void setBuildNewUserRoI(bool b);

    virtual bool isUserRegionOfInterestEnabled() const OVERRIDE FINAL;
    virtual RectD getUserRegionOfInterest() const OVERRIDE FINAL;

    void setUserRoI(const RectD & r);

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;

    /**
     * @brief update()
     **/
    virtual void redraw() OVERRIDE FINAL;
    
    /**
     * @brief updateGL();
     **/
    virtual void redrawNow() OVERRIDE FINAL;

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
    virtual void getTextureColorAt(int x,int y,double* r,double *g,double *b,double *a) OVERRIDE FINAL;
    ViewerInstance* getInternalNode() const;
    ViewerTab* getViewerTab() const;

    /**
     * @brief can only be called on the main-thread
     **/
    void setGain(double d);
    
    void setGamma(double g);

    void setLut(int lut);

    bool isWipeHandleVisible() const;

    void setZoomOrPannedSinceLastFit(bool enabled);

    bool getZoomOrPannedSinceLastFit() const;

    virtual Natron::ViewerCompositingOperatorEnum getCompositingOperator() const OVERRIDE FINAL;
    virtual void setCompositingOperator(Natron::ViewerCompositingOperatorEnum op) OVERRIDE FINAL;

    ///Not MT-Safe
    void getSelectionRectangle(double &left,double &right,double &bottom,double &top) const;

    /**
     * @brief Must save all relevant OpenGL bits so that they can be restored as-is after the draw action of a plugin.
     **/
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    
    /**
     * @brief Must restore all OpenGL bits saved in saveOpenGLContext()
     **/
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    
    /**
     * @brief Called by the Histogram when it wants to refresh. It returns a pointer to the last
     * rendered image by the viewer. It doesn't re-render the image if it is not present.
     **/
    void getLastRenderedImage(int textureIndex, std::list<boost::shared_ptr<Natron::Image> >* ret) const;
    
    void getLastRenderedImageByMipMapLevel(int textureIndex,unsigned int mipMapLevel, std::list<boost::shared_ptr<Natron::Image> >* ret) const;

    /**
     * @brief Get the color of the currently displayed image at position x,y.
     * @param forceLinear If true, then it will not use the viewer current colorspace
     * to get r,g and b values, otherwise the color returned will be in the same color-space
     * than the one chosen by the user on the gui.
     * X and Y are in CANONICAL COORDINATES
     * @return true if the point is inside the image and colors were set
     **/
    bool getColorAt(double x, double y, bool forceLinear, int textureIndex, float* r,
                    float* g, float* b, float* a,unsigned int* mipMapLevel) WARN_UNUSED_RETURN;
    
    // same as getColor, but computes the mean over a given rectangle
    bool getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                        bool forceLinear, int textureIndex, float* r, float* g, float* b, float* a, unsigned int* mipMapLevel);
    
    
    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL;
    
    ///same as getMipMapLevel but with the zoomFactor taken into account
    int getMipMapLevelCombinedToZoomFactor() const WARN_UNUSED_RETURN;
    
    virtual int getCurrentlyDisplayedTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    QPointF toZoomCoordinates(const QPointF& position) const;
    
Q_SIGNALS:

    /**
     *@brief Signal emitted when the current zoom factor changed.
     **/
    void zoomChanged(int v);

    /**
     * @brief Emitted when the image texture changes.
     **/
    void imageChanged(int texIndex,bool hasImageBackEnd);

    /**
     * @brief Emitted when the selection rectangle has changed.
     * @param onRelease When true, this signal is emitted on the mouse release event
     * which means this is the last selection desired by the user.
     * Receivers can either update the selection always or just on mouse release.
     **/
    void selectionRectangleChanged(bool onRelease);

    void selectionCleared();
    


private:
    /**
     *@brief The paint function. That's where all the drawing is done.
     **/
    virtual void paintGL() OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;    
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void tabletEvent(QTabletEvent* e) OVERRIDE FINAL;
    
    /**
     *@brief initiliazes OpenGL context related stuff. This is called once after widget creation.
     **/
    virtual void initializeGL() OVERRIDE FINAL;

    /**
     *@brief Handles the resizing of the viewer
     **/
    virtual void resizeGL(int width,int height) OVERRIDE FINAL;

private:
    
    bool penMotionInternal(int x, int y, double pressure, double timestamp, QInputEvent* event);

    /**
     * @brief Returns the OpenGL handle of the PBO at the given index.
     * If PBO at the given index doesn't exist, this function will create it.
     **/
    GLuint getPboID(int index);


    void populateMenu();

    /**
     *@brief Prints a message if the current frame buffer is incomplete.
     * where will be printed to indicate a function. If silent is off,
     * this function will report even when the frame buffer is complete.
     **/
    void checkFrameBufferCompleteness(const char where[],bool silent = true);

    /**
     *@brief Initialises shaders. If they were initialized ,returns instantly.
     **/
    void initShaderGLSL(); // init shaders


    enum DrawPolygonModeEnum
    {
        eDrawPolygonModeWhole = 0,
        eDrawPolygonModeWipeLeft,
        eDrawPolygonModeWipeRight
    };

    /**
     *@brief Makes the viewer display black only.
     **/
    void clearViewer();

    /**
     *@brief Returns !=0 if the extension given by its name is supported by this OpenGL version.
     **/
    int isExtensionSupported(const char* extension);

    /**
     *@brief Called inside paintGL(). It will draw all the overlays. 
     **/
    void drawOverlay(unsigned int mipMapLevel);

    /**
     * @brief Called by drawOverlay to draw the user region of interest.
     **/
    void drawUserRoI();

    void drawPickerRectangle();

    void drawPickerPixel();

    void drawWipeControl();

    /**
     *@brief Draws the persistent message if it is on.
     **/
    void drawPersistentMessage();

    bool isNearByUserRoITopEdge(const RectD & roi,
                                const QPointF & zoomPos,
                                double zoomScreenPixelWidth,
                                double zoomScreenPixelHeight);

    bool isNearByUserRoIRightEdge(const RectD & roi,
                                  const QPointF & zoomPos,
                                  double zoomScreenPixelWidth,
                                  double zoomScreenPixelHeight);

    bool isNearByUserRoILeftEdge(const RectD & roi,
                                 const QPointF & zoomPos,
                                 double zoomScreenPixelWidth,
                                 double zoomScreenPixelHeight);


    bool isNearByUserRoIBottomEdge(const RectD & roi,
                                   const QPointF & zoomPos,
                                   double zoomScreenPixelWidth,
                                   double zoomScreenPixelHeight);

    bool isNearByUserRoI(double x,
                         double y,
                         const QPointF & zoomPos,
                         double zoomScreenPixelWidth,
                         double zoomScreenPixelHeight);
    
    void updateInfoWidgetColorPicker(const QPointF & imgPos,
                                     const QPoint & widgetPos);

    void updateInfoWidgetColorPickerInternal(const QPointF & imgPos,
                                     const QPoint & widgetPos,
                                     int width,
                                     int height,
                                     const RectD & rod, // in canonical coordinates
                                     const RectD & dispW, // in canonical coordinates
                                     int texIndex);
    void updateRectangleColorPicker();
    void updateRectangleColorPickerInternal();
    
    
    /**
     * @brief X and Y are in widget coords!
     **/
    bool pickColor(double x,double y);
    bool pickColorInternal(double x, double y);
    

    static double currentTimeForEvent(QInputEvent* e);

    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
};


#endif // GLVIEWER_H

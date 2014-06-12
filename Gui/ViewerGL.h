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

#ifndef NATRON_GUI_VIEWERGL_H_
#define NATRON_GUI_VIEWERGL_H_

#include <vector>
#include <utility>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Rect.h"
#include "Engine/OpenGLViewerI.h"
#include "Global/Macros.h"

class QKeyEvent;
class QEvent;
class QMenu;
class QGLShaderProgram;

namespace Natron {
    class ChannelSet;
}
class InfoViewerWidget;
class AppInstance;
class ViewerInstance;
class ViewerTab;
class ImageInfo;
struct TextureRect;
class Format;

/**
 *@class ViewerGL
 *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
 *OpenGL related code as well as user events.
 **/
class ViewerGL : public QGLWidget , public OpenGLViewerI
{
    Q_OBJECT
    
public:
    
    
    /*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting.
     When constructing a viewer for the 1st time in the app, you must pass a NULL shareWidget. Otherwise,you
     can pass a pointer to the 1st viewer you created. It allows the viewers to share the same OpenGL context.
     */
    explicit ViewerGL(ViewerTab* parent, const QGLWidget* shareWidget = NULL);
    
    
    virtual ~ViewerGL() OVERRIDE;
    
    virtual QSize sizeHint() const OVERRIDE FINAL;
    
    const QFont& textFont() const;

    void setTextFont(const QFont& f);


    /**
     *@returns Returns true if the viewer is displaying something.
     **/
    bool displayingImage() const;
    
    
    /**
     *@returns Returns a const reference to the dataWindow of the currentFrame(BBOX)
     **/
    // MT-SAFE: don't return a reference
    RectI getRoD(int textureIndex) const ;
    
    
    /**
     *@returns Returns the displayWindow of the currentFrame(Resolution)
     **/
    // MT-SAFE: don't return a reference
    Format getDisplayWindow() const;
    
    
    void setClipToDisplayWindow(bool b);
    
    virtual bool isClippingImageToProjectWindow() const OVERRIDE FINAL;
    
    /**
     *@brief Saves the OpenGL context so it can be restored later-on .
     **/
    void saveGLState();
    
    /**
     *@brief Restores the OpenGL context to the state it was when calling ViewerGL::saveGLState().
     **/
    void restoreGLState();

    
    OpenGLViewerI::BitDepth getBitDepth() const OVERRIDE FINAL;
    
    /**
     *@brief Hack to allow the resizeEvent to be publicly used elsewhere.
     *It calls QGLWidget::resizeEvent(QResizeEvent*).
     **/
    virtual void resizeEvent(QResizeEvent* event) OVERRIDE FINAL;
    
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
    double getZoomFactor() const;

    /**
     * @brief Returns the rectangle of the image displayed by the viewer
     **/
    virtual RectI getImageRectangleDisplayed(const RectI& imageRoD) OVERRIDE FINAL;

    /**
     *@brief Set the pointer to the InfoViewerWidget. This is called once after creation
     *of the ViewerGL.
     **/
    void setInfoViewer(InfoViewerWidget* i,int textureIndex);
    
    /**
     *@brief Handy function that zoom automatically the viewer so it fit
     *the displayWindow  entirely in the viewer
     **/
    virtual void fitImageToFormat() OVERRIDE FINAL;
    
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
     *used texture. 
     * It does:
     * 1) glMapBuffer
     * 2) memcpy to copy data from RAM to GPU
     * 3) glUnmapBuffer
     * 4) glTexSubImage2D or glTexImage2D depending whether we resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const unsigned char* ramBuffer, size_t bytesCount, const TextureRect& region, double gain, double offset, int lut, int pboIndex,unsigned int mipMapLevel,int textureIndex) OVERRIDE FINAL;
    
    
    virtual void disconnectInputTexture(int textureIndex) OVERRIDE FINAL;
    /**
     *@returns Returns true if the graphic card supports GLSL.
     **/
    virtual bool supportsGLSL() const OVERRIDE FINAL;
    
    /**
     *@brief Disconnects the viewer.
     *Clears out the viewer and reset the viewer infos. Note that calling this
     *function while the engine is processing will abort the engine.
     **/
    void disconnectViewer();
    
    void setPersistentMessage(int type,const QString& message);
    
    void clearPersistentMessage();
    
    public slots:
   
    
    /**
     *@brief Slot used by the GUI to zoom at the current center the viewport.
     *@param v[in] an integer holding the desired zoom factor (in percent).
     **/
    void zoomSlot(int v);
    
    /**
     *@brief Convenience function. See ViewerGL::zoomSlot(int)
     **/
    void zoomSlot(double v){zoomSlot((int)v);}
    
    /**
     *@brief Convenience function. See ViewerGL::zoomSlot(int)
     *It parses the Qstring and removes the '%' character
     **/
    void zoomSlot(QString);
    
    void setRegionOfDefinition(const RectI& rod,int textureIndex);
    
    virtual void updateColorPicker(int textureIndex,int x = INT_MAX,int y = INT_MAX) OVERRIDE FINAL;
    
        
    void clearColorBuffer(double r = 0.,double g = 0.,double b = 0.,double a = 1.);
    
    void toggleOverlays();
        
    void onProjectFormatChanged(const Format& format);
    
    virtual void makeOpenGLcontextCurrent() OVERRIDE FINAL;
    
    virtual void onViewerNodeNameChanged(const QString& name) OVERRIDE FINAL;
    
    virtual void removeGUI() OVERRIDE FINAL;

    virtual int getCurrentView() const OVERRIDE FINAL;
    
    /**
     * @brief Reset the wipe position so it is in the center of the B input.
     * If B input is disconnected it goes in the middle of the A input.
     * Otherwise it goes in the middle of the project window
     **/
    void resetWipeControls();
    
public:

    void renderText(int x, int y, const QString &string,const QColor& color,const QFont& font);

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomPAR) const;
    
    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomPAR);
    
    void setUserRoIEnabled(bool b);
    
    virtual bool isUserRegionOfInterestEnabled() const OVERRIDE FINAL;
    
    virtual RectI getUserRegionOfInterest() const OVERRIDE FINAL;
    
    void setUserRoI(const RectI& r);

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
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL ;

    /**
    * @brief Returns the pixel scale of the viewport.
    **/
    virtual void getPixelScale(double& xScale, double& yScale) const  OVERRIDE FINAL;

    /**
    * @brief Returns the colour of the background (i.e: clear color) of the viewport.
    **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL ;
    
    ViewerInstance* getInternalNode() const;
    
    ViewerTab* getViewerTab() const;
    
    /**
     * @brief can only be called on the main-thread
     **/
    void setGain(double d);
    
    void setLut(int lut);
    
    bool isWipeHandleVisible() const;
    
    void setZoomOrPannedSinceLastFit(bool enabled);
    
    bool getZoomOrPannedSinceLastFit() const;
    
    virtual Natron::ViewerCompositingOperator getCompositingOperator() const OVERRIDE FINAL;
    
signals:
    /**
     *@brief Signal emitted when the mouse position changed on the viewport.
     **/
    void infoMousePosChanged();
    
    /**
     *@brief Signal emitted when the mouse position changed on the viewport.
     **/
    void infoColorUnderMouseChanged();
    
    /**
     *@brief Signal emitted when the current display window changed.
     **/
    void infoResolutionChanged();
    
    /**
     *@brief Signal emitted when the current data window changed.
     **/
    void infoDataWindow1Changed();
    void infoDataWindow2Changed();
    /**
     *@brief Signal emitted when the current zoom factor changed.
     **/
    void zoomChanged(int v);
    
    /**
     * @brief Emitted when the image texture changes.
     **/
    void imageChanged(int texIndex);

private:
    /**
     *@brief The paint function. That's where all the drawing is done.
     **/
    virtual void paintGL() OVERRIDE FINAL;
    
    virtual void mousePressEvent(QMouseEvent *event) OVERRIDE FINAL;
    
    virtual void mouseDoubleClickEvent(QMouseEvent* event) OVERRIDE FINAL;
    
    virtual void mouseReleaseEvent(QMouseEvent *event) OVERRIDE FINAL;
    
    virtual void mouseMoveEvent(QMouseEvent *event) OVERRIDE FINAL;
    
    virtual void wheelEvent(QWheelEvent *event) OVERRIDE FINAL;
    
    virtual void focusInEvent(QFocusEvent *event) OVERRIDE FINAL;
    
    virtual void focusOutEvent(QFocusEvent *event) OVERRIDE FINAL;

    virtual void enterEvent(QEvent *event) OVERRIDE FINAL;

    virtual void leaveEvent(QEvent *event) OVERRIDE FINAL;

    virtual void keyPressEvent(QKeyEvent* event) OVERRIDE FINAL;
    
    virtual void keyReleaseEvent(QKeyEvent* event) OVERRIDE FINAL;
    
    /**
     *@brief initiliazes OpenGL context related stuff. This is called once after widget creation.
     **/
    virtual void initializeGL() OVERRIDE FINAL;
    
    /**
     *@brief Handles the resizing of the viewer
     **/
    virtual void resizeGL(int width,int height) OVERRIDE FINAL;
    
private:
    
    /**
     * @brief Returns the OpenGL handle of the PBO at the given index.
     * If PBO at the given index doesn't exist, this function will create it.
     **/
    GLuint getPboID(int index);
    
    
    void populateMenu();
    
    /**
     *@brief Prints a message if the current frame buffer is incomplete.
     *where will be printed to indicate a function. If silent is off,
     *this function will report even when the frame buffer is complete.
     **/
    void checkFrameBufferCompleteness(const char where[],bool silent=true);
    
    /**
     *@brief Checks extensions and init glew on windows. Called by  initializeGL()
     **/
    void initAndCheckGlExtensions ();
    
    /**
     *@brief Initialises shaders. If they were initialized ,returns instantly.
     **/
    void initShaderGLSL(); // init shaders
    
    
    enum DrawPolygonMode {
        ALL_PLANE = 0,
        WIPE_LEFT_PLANE,
        WIPE_RIGHT_PLANE
    };
    
    /**
     *@brief Fill the rendering VAO with vertices and texture coordinates
     *that depends upon the currently displayed texture.
     **/
    void drawRenderingVAO(unsigned int mipMapLevel,int textureIndex,DrawPolygonMode polygonMode);
    
    /**
     *@brief Makes the viewer display black only.
     **/
    void clearViewer();
    
    /**
     *@brief Returns !=0 if the extension given by its name is supported by this OpenGL version.
     **/
    int isExtensionSupported(const char* extension);
    
    QString getOpenGLVersionString() const;
    
    QString getGlewVersionString() const;
    
    
    /**
     *@brief Called inside paintGL(). It will draw all the overlays. It also calls
     *VideoEngine::drawOverlay()
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

    bool isNearByUserRoITopEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoIRightEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoILeftEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoIBottomEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoIMiddleHandle(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoITopLeft(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoITopRight(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoIBottomRight(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);
    
    bool isNearByUserRoIBottomLeft(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);

    void updateInfoWidgetColorPicker(const QPointF& imgPos,const QPoint& widgetPos,int width,int height,
                                     const RectI& rod,const RectI& dispW,int texIndex);
    void updateRectangleColorPicker();
    /**
     * @brief X and Y are in widget coords!
     **/
    bool pickColor(double x,double y);
    
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
};


#endif // GLVIEWER_H

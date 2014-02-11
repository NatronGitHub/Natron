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

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>

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
class RectI;
class TextureRect;
class Format;

/**
 *@class ViewerGL
 *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
 *OpenGL related code as well as user events.
 **/
class ViewerGL : public QGLWidget
{
    Q_OBJECT
    
public:
    
    
    /*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting.
     When constructing a viewer for the 1st time in the app, you must pass a NULL shareWidget. Otherwise,you
     can pass a pointer to the 1st viewer you created. It allows the viewers to share the same OpenGL context.
     */
    explicit ViewerGL(ViewerTab* parent, const QGLWidget* shareWidget = NULL);
    
    
    virtual ~ViewerGL() OVERRIDE;
    
    QSize sizeHint() const;
    
    const QFont& textFont() const;

    void setTextFont(const QFont& f);

    
    /**
     *@brief Toggles on/off the display on the viewer. If d is false then it will
     *render black only.
     **/
    void setDisplayingImage(bool d);

    /**
     *@returns Returns true if the viewer is displaying something.
     **/
    bool displayingImage() const;
    
    
    /**
     *@returns Returns a const reference to the dataWindow of the currentFrame(BBOX)
     **/
    const RectI& getRoD() const ;
    
    /**
     *@returns Returns a const reference to the displayWindow of the currentFrame(Resolution)
     **/
    const Format& getDisplayWindow() const;
    
    void setRod(const RectI& rod);
        
    void setClipToDisplayWindow(bool b);
    
    bool isClippingToDisplayWindow() const;
    
    /**
     *@brief Saves the OpenGL context so it can be restored later-on .
     **/
    void saveGLState();
    
    /**
     *@brief Restores the OpenGL context to the state it was when calling ViewerGL::saveGLState().
     **/
    void restoreGLState();
        
    /**
     *@returns Returns 1.f if the viewer is using 8bit textures.
     *Returns 0.f if the viewer is using 32bit f.p textures.
     **/
    int bitDepth() const;
    
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
    double getZoomFactor();

    /**
     * @brief Returns the rectangle of the image displayed by the viewer
     **/
    RectI getImageRectangleDisplayed(const RectI& imageRoD);
    
    /**
     *@brief Computes the viewport coordinates of the point passed in parameter.
     *This function actually does the projection to retrieve the position;
     *@param x[in] The x coordinate of the point in image coordinates.
     *@param y[in] The y coordinates of the point in image coordinates.
     *@returns Returns the viewport coordinates mapped equivalent of (x,y).
     **/
    QPointF toWidgetCoordinates(double x,double y);
    
    /**
     *@brief Computes the image coordinates of the point passed in parameter.
     *This function actually does the unprojection to retrieve the position.
     *@param x[in] The x coordinate of the point in viewport coordinates.
     *@param y[in] The y coordinates of the point in viewport coordinates.
     *@returns Returns the image coordinates mapped equivalent of (x,y) as well as the depth.
     **/
    //QVector3D toImgCoordinates_slow(int x,int y);
    
    /**
     *@brief Computes the image coordinates of the point passed in parameter.
     *This is a fast in-line method much faster than toImgCoordinates_slow().
     *This function actually does the unprojection to retrieve the position.
     *@param x[in] The x coordinate of the point in viewport coordinates.
     *@param y[in] The y coordinates of the point in viewport coordinates.
     *@returns Returns the image coordinates mapped equivalent of (x,y).
     **/
    QPointF toImgCoordinates_fast(double x, double y);
    
    
    /**
     *@brief Set the pointer to the InfoViewerWidget. This is called once after creation
     *of the ViewerGL.
     **/
    void setInfoViewer(InfoViewerWidget* i);
    
    /**
     *@brief Handy function that zoom automatically the viewer so it fit
     *the displayWindow  entirely in the viewer
     **/
    void fitToFormat(const Format &rod);
    
    /**
     *@returns Returns a pointer to the current viewer infos.
     **/
    const ImageInfo& getCurrentViewerInfos() const;
    
    ViewerTab* getViewerTab() const;
    
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
    void transferBufferFromRAMtoGPU(const unsigned char* ramBuffer, size_t bytesCount, const TextureRect& region,int pboIndex);
    
    
    /**
     *@returns Returns true if the graphic card supports GLSL.
     **/
    bool supportsGLSL();
    
    /**
     *@brief Disconnects the viewer.
     *Clears out the viewer and reset the viewer infos. Note that calling this
     *function while the engine is processing will abort the engine.
     **/
    void disconnectViewer();
    
    void stopDisplayingProgressBar();
    
    void backgroundColor(double &r,double &g,double &b);
    
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
    
    
    void updateColorPicker(int x = INT_MAX,int y = INT_MAX);
    
    
    /**
     *@brief Updates the Viewer with what has been computed so far in the texture.
     **/
    //void updateProgressOnViewer(const RectI& region,int y , int texY);
    
    void doSwapBuffers();
    
    void clearColorBuffer(double r = 0.,double g = 0.,double b = 0.,double a = 1.);
    
    void toggleOverlays();
    
    void renderText(int x, int y, const QString &string,const QColor& color,const QFont& font);
    
    void onProjectFormatChanged(const Format& format);
    
    void getProjection(double &left,double &bottom,double &zoomFactor) const;
    
    void setProjection(double left,double bottom,double zoomFactor);
    
    void setUserRoIEnabled(bool b);
    
    bool isUserRoIEnabled() const;
    
    const RectI& getUserRoI() const;
    
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
    void infoDataWindowChanged();
    
    /**
     *@brief Signal emitted when the current zoom factor changed.
     **/
    void zoomChanged(int v);
    

private:
    /**
     *@brief The paint function. That's where all the drawing is done.
     **/
    virtual void paintGL() OVERRIDE FINAL;
    
    virtual void mousePressEvent(QMouseEvent *event) OVERRIDE FINAL;
    
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
    
    /**
     *@brief Starts using the RGB shader to display the frame
     **/
    void activateShaderRGB();
    
    /**
     *@brief Fill the rendering VAO with vertices and texture coordinates
     *that depends upon the currently displayed texture.
     **/
    void drawRenderingVAO();
    
    /**
     *@brief Makes the viewer display black only.
     **/
    void clearViewer();
    
    /**
     *@brief Returns !=0 if the extension given by its name is supported by this OpenGL version.
     **/
    int isExtensionSupported(const char* extension);
    
    /**
     *@brief Initialises the black texture, drawn when the viewer is disconnected. It allows
     *the coordinate tracker on the GUI to still work even when the viewer is not connected.
     **/
    void initBlackTex();// init the black texture when viewer is disconnected
    
    
    /**
     *@brief Called inside paintGL(). It will draw all the overlays. It also calls
     *VideoEngine::drawOverlay()
     **/
    void drawOverlay();
    
    /**
     * @brief Called by drawOverlay to draw the user region of interest.
     **/
    void drawUserRoI();
    
    /**
     *@brief Draws the persistent message if it is on.
     **/
    void drawPersistentMessage();
    
    /**
     *@brief draws the progress bar
     **/
    void drawProgressBar();
    
    /**
     *@brief Resets the mouse position
     **/
    void resetMousePos();
    
    bool isNearByUserRoITopEdge(const QPoint& mousePos);
    
    bool isNearByUserRoIRightEdge(const QPoint& mousePos);
    
    bool isNearByUserRoILeftEdge(const QPoint& mousePos);
    
    bool isNearByUserRoIBottomEdge(const QPoint& mousePos);
    
    bool isNearByUserRoIMiddleHandle(const QPoint& mousePos);
    
    bool isNearByUserRoITopLeft(const QPoint& mousePos);
    
    bool isNearByUserRoITopRight(const QPoint& mousePos);
    
    bool isNearByUserRoIBottomRight(const QPoint& mousePos);
    
    bool isNearByUserRoIBottomLeft(const QPoint& mousePos);

    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
};


#endif // GLVIEWER_H

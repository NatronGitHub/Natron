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
class Format;
class TextureRect;

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
    
    
    virtual ~ViewerGL();
    
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
    // FIXME-seeabove: why a float to really represent an enum????
    float byteMode() const;
    
    /**
     *@brief Hack to allow the resizeEvent to be publicly used elsewhere.
     *It calls QGLWidget::resizeEvent(QResizeEvent*).
     **/
    virtual void resizeEvent(QResizeEvent* event);
    
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
     *@brief Computes what are the rows that should be displayed on viewer
     *for the given displayWindow with the  current zoom factor and  current zoom center.
     *The rows will be stored from bottom to top. The values are returned
     *as a vector of image coordinates.
     *This function does not use any OpenGL function, so it can be safely called in
     *a thread that does not own the context.
     *@return Returns a pair with the first row index and the last row indexes.
     **/
    std::pair<int,int> computeRowSpan(int bottom,int top, std::vector<int>* rows);
    
    /**
     *@brief same as computeRowSpan but for columns.
     **/
    std::pair<int,int> computeColumnSpan(int left,int right, std::vector<int>* columns);
    
    /**
     *@brief Computes the viewport coordinates of the point passed in parameter.
     *This function actually does the projection to retrieve the position;
     *@param x[in] The x coordinate of the point in image coordinates.
     *@param y[in] The y coordinates of the point in image coordinates.
     *@returns Returns the viewport coordinates mapped equivalent of (x,y).
     **/
    QPoint toWidgetCoordinates(double x,double y);
    
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
    QPointF toImgCoordinates_fast(int x,int y);
    
    /**
     *@brief Returns the rgba components of the pixel located at position (x,y) in viewport coordinates.
     *This function unprojects the x and y coordinates to retrieve the OpenGL coordinates
     *of the point. Then it makes a call to glReadPixels to retrieve the intensities stored at this location.
     *This function may slow down a little bit the rendering pipeline since it forces the OpenGL context to
     *synchronize.
     *@param x[in] The x coordinate of the pixel in viewport coordinates.
     *@param y[in] The y coordinate of the pixel in viewport coordinates.
     *@returns Returns the RGBA components of the pixel at (x,y).
     **/
    QVector4D getColorUnderMouse(int x,int y);
    
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
    
    /**
     *@brief set the channels the viewer should display
     **/
    void setDisplayChannel(const Natron::ChannelSet& channels,bool yMode = false);
    
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
    void updateProgressOnViewer(const TextureRect& region,int y , int texY);
    
    void doSwapBuffers();
    
    void clearColorBuffer(double r = 0.,double g = 0.,double b = 0.,double a = 1.);
    
    void toggleOverlays();
    
    void renderText(int x, int y, const QString &string,const QColor& color,const QFont& font);
    
    void onProjectFormatChanged(const Format& format);
    
    
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
    

    protected :
    /**
     *@brief The paint function. That's where all the drawing is done.
     **/
    virtual void paintGL();
    
    virtual void mousePressEvent(QMouseEvent *event);
    
    virtual void mouseReleaseEvent(QMouseEvent *event);
    
    virtual void mouseMoveEvent(QMouseEvent *event);
    
    virtual void wheelEvent(QWheelEvent *event);
    
    virtual void enterEvent(QEvent *event);
    
    virtual void leaveEvent(QEvent *event);
    
    virtual void keyPressEvent(QKeyEvent* event);
    
    virtual void keyReleaseEvent(QKeyEvent* event);
    
    /**
     *@brief initiliazes OpenGL context related stuff. This is called once after widget creation.
     **/
    virtual void initializeGL();
    
    /**
     *@brief Handles the resizing of the viewer
     **/
    virtual void resizeGL(int width,int height);
    
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
     *@brief Starts using the luminance/chroma shader
     **/
    void activateShaderLC();
    
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

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
};


#endif // GLVIEWER_H

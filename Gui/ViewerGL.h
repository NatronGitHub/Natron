//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GUI_VIEWERGL_H_
#define POWITER_GUI_VIEWERGL_H_

#include <cmath>
#include <vector>
#include <utility>
#include <cassert>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
#include <QtGui/QVector4D>
#include <QMutex>
#include <QWaitCondition>

#include "Engine/ImageInfo.h"
#include "Engine/Format.h"
#include "Engine/ChannelSet.h"
#include "Gui/Texture.h"

class QKeyEvent;
class QEvent;
class QMenu;
class QGLShaderProgram;
class FTTextureFont;
namespace Powiter {
    namespace Color {
        class Lut;
    }
}
class InfoViewerWidget;
class AppInstance;
class ViewerNode;
class ViewerTab;

#ifndef POWITER_DEBUG
#define checkGLErrors() ((void)0)
#define assert_checkGLErrors() ((void)0)
#else
#define checkGLErrors() \
{ \
    GLenum _glerror_ = glGetError(); \
    if(_glerror_ != GL_NO_ERROR) { \
        std::cout << "GL_ERROR :" << __FILE__ << " "<< __LINE__ << " " << gluErrorString(_glerror_) << std::endl; \
        } \
        }
#define assert_checkGLErrors() \
{ \
    GLenum _glerror_ = glGetError(); \
    if(_glerror_ != GL_NO_ERROR) { \
        std::cout << "GL_ERROR :" << __FILE__ << " "<< __LINE__ << " " << gluErrorString(_glerror_) << std::endl; abort(); \
        } \
        }
#endif

        
        /**
         *@class ViewerGL
         *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
         *OpenGL related code as well as user events.
         **/
        class ViewerGL : public QGLWidget
        {
            Q_OBJECT
                        
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
            class ZoomContext{
                
            public:
                
                ZoomContext():_bottom(0.),_left(0.),_zoomFactor(1.)
                {}
                
                QPoint _oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
                double _bottom; /// the bottom edge of orthographic projection
                double _left; /// the left edge of the orthographic projection
                double _zoomFactor; /// the zoom factor applied to the current image
                                                
                /*!< the level of zoom used to display the frame*/
                void setZoomFactor(double f){assert(f>0.); _zoomFactor = f;}
                
                double getZoomFactor() const {return _zoomFactor;}
            };
            
            /**
             *@enum MOUSE_STATE
             *@brief basic state switching for mouse events
             **/
            enum MOUSE_STATE{DRAGGING,UNDEFINED};
            
            GLuint _pboIds[2]; /*!< PBO's id's used by the OpenGL context*/
            
            //   GLuint _vaoId; /*!< VAO holding the rendering VBOs for texture mapping.*/
            
            GLuint _vboVerticesId; /*!< VBO holding the vertices for the texture mapping*/
            
            GLuint _vboTexturesId; /*!< VBO holding texture coordinates*/

            GLuint _iboTriangleStripId; /*!< IBOs holding vertices indexes for triangle strip sets*/
                        
            Texture* _defaultDisplayTexture;/*!< A pointer to the current texture used to display.*/
            
            Texture* _blackTex;/*!< the texture used to render a black screen when nothing is connected.*/
            
            QGLShaderProgram* shaderRGB;/*!< The shader program used to render RGB data*/
            QGLShaderProgram* shaderLC;/*!< The shader program used to render YCbCr data*/
            QGLShaderProgram* shaderBlack;/*!< The shader program used when the viewer is disconnected.*/
            
            bool _shaderLoaded;/*!< Flag to check whether the shaders have already been loaded.*/
            
            // FIXME: why a float to really represent an enum????
            float _lut; /*!< a value coding the current color-space used to render.
                         0 = NONE , 1 = sRGB , 2 = Rec 709*/
            
            const Powiter::Color::Lut* _colorSpace;/*!< The lut used to do the viewer colorspace conversion when we can't use shaders*/
            
            QWaitCondition _usingColorSpaceCondition;
            QMutex _usingColorSpaceMutex;
            int _usingColorSpaceCounter;
            
            InfoViewerWidget* _infoViewer;/*!< Pointer to the info bar below the viewer holding pixel/mouse/format related infos*/
            
            ViewerTab* _viewerTab;/*!< Pointer to the viewer tab GUI*/
            
            double exposure ;/*!< Current exposure setting, all pixels are multiplied
                             by pow(2,expousre) before they appear on the screen.*/
            
            ImageInfo _currentViewerInfos;/*!< Pointer to the ViewerInfos  used for rendering*/
            
            ImageInfo _blankViewerInfos;/*!< Pointer to the infos used when the viewer is disconnected.*/
            
            bool _displayingImage;/*!< True if the viewer is connected and not displaying black.*/
            bool _must_initBlackTex;

            MOUSE_STATE _ms;/*!< Holds the mouse state*/
                        
            std::vector<int> _textureColumns; /*!< The last columns computed by computeColumnSpan. This member is
                                               used in the convertRowToFitTextureBGRA function.*/
            
            ZoomContext _zoomCtx;/*!< All zoom related variables are packed into this object*/
            
            QString _resolutionOverlay;/*!< The string holding the resolution overlay, e.g: "1920x1080"*/
            
            QString _btmLeftBBOXoverlay;/*!< The string holding the bottom left corner coordinates of the dataWindow*/
            
            QString _topRightBBOXoverlay;/*!< The string holding the top right corner coordinates of the dataWindow**/
            
            bool _overlay;/*!< True if the user enabled overlay dispay*/
            
            bool _hasHW;/*!< True if the user has a GLSL version supporting everything requested.*/
            
            char* frameData;/*!< Pointer to the buffer holding the data computed for the last frame.
                             This buffer is the currently mapped PBO.*/
            
            bool _pBOmapped; /*!< True if the main PBO (_pbosId[0]) is currently mapped*/
            
            // FIXME-seeabove: why a float to really represent an enum????
            float _displayChannels;
                        
            bool _drawProgressBar;
            
            bool _updatingTexture;
            
            int _progressBarY;
            
            QColor _clearColor;
            
            QMenu* _menu;
            
            FTTextureFont* _font;
            
            bool _clipToDisplayWindow;
            
        public:
            
            
            /*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting.
             When constructing a viewer for the 1st time in the app, you must pass a NULL shareWidget. Otherwise,you
             can pass a pointer to the 1st viewer you created. It allows the viewers to share the same OpenGL context.
             */
            explicit ViewerGL(ViewerTab* parent, const QGLWidget* shareWidget=NULL);
            explicit ViewerGL(const QGLFormat& format,ViewerTab* parent, const QGLWidget* shareWidget=NULL);
            explicit ViewerGL(QGLContext* context,ViewerTab* parent, const QGLWidget* shareWidget=NULL);
            
            virtual ~ViewerGL();
            
            QSize sizeHint() const;
            
            /**
             *@brief This function is called by the viewer node. It fills 1 scan-line in the
             *output buffer using the buffers in parameters. Since each scan-line lies at a
             *different offset in the output buffer, this function is thread-safe.
             *Internally this function calls the appropriate function to fill the output buffer
             *depending on the bit-depth currently used by the viewer.
             *@param r A pointer to the red component of the row to draw.
             *@param g A pointer to the green component of the row to draw.
             *@param b A pointer to the blue component of the row to draw.
             *@param a A pointer to the alpha component of the row to draw.
             */
            void drawRow(const float* r,const float* g,const float* b,const float* a,int zoomedY);
            
            /**
             *@brief Toggles on/off the display on the viewer. If d is false then it will
             *render black only.
             **/
            void setDisplayingImage(bool d) {
                _displayingImage = d;
                if (!_displayingImage) {
                    _must_initBlackTex = true;
                };
            }
            
            /**
             *@returns Returns true if the viewer is displaying something.
             **/
            bool displayingImage() const { return _displayingImage; }
            
            
            /**
             *@returns Returns a const reference to the dataWindow of the currentFrame(BBOX)
             **/
            const Box2D& getRoD() const ;
            
            /**
             *@returns Returns a const reference to the displayWindow of the currentFrame(Resolution)
             **/
            const Format& getDisplayWindow() const;
            
            void setRod(const Box2D& rod);
            
            
            void setClipToDisplayWindow(bool b) ;
            
            bool isClippingToDisplayWindow() const {return _clipToDisplayWindow;}
            
            /**
             *@brief Saves the OpenGL context so it can be restored later-on .
             **/
            void saveGLState();
            
            /**
             *@brief Restores the OpenGL context to the state it was when calling ViewerGL::saveGLState().
             **/
            void restoreGLState();
            
            /**
             *@brief Makes the OpenGL current to the current thread. See the Qt doc for a full explanation.
             *This function is quite faster than the one used by Qt as it does nothing is the current context
             *is already this one.
             **/
            void makeCurrent();
            
            /**
             *@returns Returns a value indicating the current viewer process used for rendering.
             *0 = NONE , 1 = sRGB , 2 = Rec 709.
             *Not that this will probably change in the future,allowing the user to add custom a viewer process.
             **/
            // FIXME-seeabove: why a float to really represent an enum????
            float lutType() const {return _lut;}
            
            /**
             *@returns Returns the current exposure setting, all pixels are multiplied
             *by pow(2,exposure) before they appear on the screen.
             **/
            double getExposure() const {return exposure;}
            
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
            double zoomedHeight(){return std::floor(getDisplayWindow().height()*_zoomCtx._zoomFactor);}
            
            /**
             *@returns Returns the width of the frame with the scale factor applied to it.
             **/
            double zoomedWidth(){return std::floor(getDisplayWindow().width()*_zoomCtx._zoomFactor);}
            
            /**
             *@returns Returns the current zoom factor that is applied to the display.
             **/
            double getZoomFactor(){return _zoomCtx.getZoomFactor();}
            
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
            QPoint toWidgetCoordinates(int x,int y);
            
            /**
             *@brief Computes the image coordinates of the point passed in parameter.
             *This function actually does the unprojection to retrieve the position.
             *@param x[in] The x coordinate of the point in viewport coordinates.
             *@param y[in] The y coordinates of the point in viewport coordinates.
             *@returns Returns the image coordinates mapped equivalent of (x,y) as well as the depth.
             **/
            QVector3D toImgCoordinates_slow(int x,int y);
            
            /**
             *@brief Computes the image coordinates of the point passed in parameter.
             *This is a fast in-line method much faster than toImgCoordinates_slow().
             *This function actually does the unprojection to retrieve the position.
             *@param x[in] The x coordinate of the point in viewport coordinates.
             *@param y[in] The y coordinates of the point in viewport coordinates.
             *@returns Returns the image coordinates mapped equivalent of (x,y).
             **/
            QPointF toImgCoordinates_fast(int x,int y){
                float w = (float)width() ;
                float h = (float)height();
                float bottom = _zoomCtx._bottom;
                float left = _zoomCtx._left;
                float top =  bottom +  h / _zoomCtx._zoomFactor;
                float right = left +  w / _zoomCtx._zoomFactor;
                return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
            }

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
            void fitToFormat(const Box2D& rod);
            
            /**
             *@returns Returns a pointer to the current viewer infos.
             **/
            const ImageInfo& getCurrentViewerInfos() const {return _currentViewerInfos;}
            
            ViewerTab* getViewerTab() const {return _viewerTab;}
            
            /**
             *@brief Turns on the overlays on the viewer.
             **/
            void turnOnOverlay(){_overlay=true;}
            
            /**
             *@brief Turns off the overlays on the viewer.
             **/
            void turnOffOverlay(){_overlay=false;}
            
            /**
             *@brief Called by the video engine to allocate the frameData.
             *@param w,h[in] The width and height of the scaled frame.
             *@returns Returns the size in bytes of the space occupied in memory by the frame.
             **/
            size_t allocateFrameStorage(int w,int h);
            
            /**
             *@brief Allocates the pbo represented by the pboID with dataSize bytes.
             This function returns a pointer to the mapping created between the GPU and the RAM.
             *@param dataSize The size in bytes that should be allocated to the PBO.
             *@param pboID The OpenGL ID of the PBO to use.
             *@returns Returns a pointer mapped in RAM to the PBO.
             **/
            void* allocateAndMapPBO(size_t dataSize,GLuint pboID);
            
            /**
             *@brief Fill the buffer dst with byteCount bytes of src (simple call to memcpy)
             **/
            void fillPBO(const char* src,void* dst,size_t byteCount);
            
            void unMapPBO();
            
            void unBindPBO();
            
            /**
             *@brief Copies the data stored in the currently mapped pbo into the currently
             *used texture. Note that it unmaps the current PBO before actually copying data
             *to the texture.
             **/
            void copyPBOToRenderTexture(const TextureRect& region);
            

            /**
             *@returns *Returns a pointer to the data of the current frame.
             **/
            const char* getFrameData() const {return frameData;}
            
            /**
             *@returns Returns the OpenGL PBO located at index. Index must be either 0 or 1.
             **/
            GLuint getPBOId(int index) const {return _pboIds[index];}
            
            
            /**
             *@returns Returns true if a pbo is currently mapped
             **/
            bool hasPBOCurrentlyMapped() const {return _pBOmapped;}
            
            /**
             *@returns A vector with the columns indexes of the full-size image that are
             *really contained in the texture.
             **/
            const std::vector<int>& getTextureColumns() const {return _textureColumns;}
            
            
            
            /**
             *@returns Returns true if the graphic card supports GLSL.
             **/
            bool hasHardware(){return _hasHW;}
            
            /**
             *@brief Disconnects the viewer.
             *Clears out the viewer and reset the viewer infos. Note that calling this
             *function while the engine is processing will abort the engine.
             **/
            void disconnectViewer();
            
            /**
             *@brief set the channels the viewer should display 
             **/
            void setDisplayChannel(const Powiter::ChannelSet& channels,bool yMode = false);
            
            void stopDisplayingProgressBar() {_drawProgressBar = false;}
            
            void backgroundColor(double &r,double &g,double &b);
            
public slots:
            /**
             *@brief Slot used by the GUI to change the current viewer process applied to all pixels.
             *@param str[in] A string whose name is a valid color-space.
             *WARNING: this function may change in the future.
             **/
            void updateColorSpace(QString str);
            
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
            
            /**
             *@brief Slot called by the GUI. It changes the current exposure settings.
             **/
            void updateExposure(double);
            
            void updateColorPicker(int x = INT_MAX,int y = INT_MAX);
            
            
            /**
             *@brief Updates the Viewer with what has been computed so far in the texture.
             **/
            void updateProgressOnViewer(const TextureRect& region,int y , int texY);
            
            void doSwapBuffers();
            
            void clearColorBuffer(double r = 0.,double g = 0.,double b = 0.,double a = 1.);
            
            void toggleOverlays(){ _overlay = ! _overlay; updateGL();}
            
            void print( int x, int y, const QString&string, QColor color);
            
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
            
            /**
             *@brief Signal emitted when the current frame changed.
             **/
            void frameChanged(int);
            
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
             *@brief Called by the contructor. It avoids re-writing the same code for all constructors.
             **/
            void initConstructor();
            
            void initTextFont();
            
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
             *@brief draws the progress bar
             **/
            void drawProgressBar();
            
            /**
             *@brief Actually converting to ARGB... but it is called BGRA by
             the texture format GL_UNSIGNED_INT_8_8_8_8_REV
             **/
            static U32 toBGRA(U32 r,U32 g,U32 b,U32 a);
            
            /**
             *@brief This function fills the member frameData with the buffer in parameters.
             *Since the frameData buffer holds the data "as seen" on the viewer (i.e with a scale-factor)
             *it will apply a "Nearest neighboor" algorithm to fill the buffer. Note that during the
             *conversion to 8bit in the viewer color-space, a dithering algorithm is used.
             *@param r[in] Points to the red component of the scan-line.
             *@param g[in] Points to the green component of the scan-line.
             *@param b[in] Points to the blue component of the scan-line.
             *@param alpha[in] Points to the alpha component of the scan-line.
             *@param columnSpan[in] The columns in the row to keep.
             *@param yOffset[in] The index of the scan-line in the scaled frame.
             **/
            void convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                            const std::vector<int>& columnSpan,int yOffset,const float* alpha);
            
            /**
             *@brief This function fills the member frameData with the buffer in parameters.
             *Since the frameData buffer holds the data "as seen" on the viewer (i.e with a scale-factor)
             *it will apply a "Nearest neighboor" algorithm to fill the buffer. This is the same function
             *as ViewerGL::convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
             *int w,int yOffset,const float* alpha) except that it does not apply any dithering nor color-space
             *since the data are stored as 32bit floating points. The color-space will be applied by the shaders.
             *@param r[in] Points to the red component of the scan-line.
             *@param g[in] Points to the green component of the scan-line.
             *@param b[in] Points to the blue component of the scan-line.
             *@param alpha[in] Points to the alpha component of the scan-line.
             *@param columnSpan[in] The columns in the row to keep.
             *@param yOffset[in] The index of the scan-line in the scaled frame.
             **/
            void convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                               const std::vector<int>& columnSpan,int yOffset,const float* alpha);
            

            /**
             *@brief Resets the mouse position
             **/
            void resetMousePos(){_zoomCtx._oldClick.setX(0); _zoomCtx._oldClick.setY(0);}
        };
#endif // GLVIEWER_H

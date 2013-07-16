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

 

 



#ifndef GL_VIEWER_HEADER__
#define GL_VIEWER_HEADER__


#include "Superviser/gl_OsDependent.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <cmath>
#include <QtOpenGL/QGLWidget>
#include <QtGui/QVector4D>
#include "Core/displayFormat.h"
#include "Core/channels.h"
#include "Gui/textRenderer.h"
#include "Gui/texturecache.h"
#include <Eigen/Dense>
#ifndef PW_DEBUG
#define checkGLErrors() ((void)0)
#else
#define checkGLErrors() \
{ \
	GLenum error = glGetError(); \
	if(error != GL_NO_ERROR) { \
        std::cout << "GL_ERROR :" << __FILE__ << " "<< __LINE__ << " " << gluErrorString(error) << std::endl; \
        } \
		}
#endif
        
        
        /**
         *@class ViewerInfos
         *@brief Holds info necessary to render like channels,display window...See the documentation of
         *Node::Info for a more complete documentation.
         **/
        class ViewerInfos : public Box2D{
            
            int _firstFrame; /*!< first frame in the sequence*/
            int _lastFrame; /*!< last frame in the sequence*/
            bool _rgbMode; /*!< true if displaying RGB image, otherwise it assumes YCbCr*/
            Format _displayWindow; /*!< display window of the data, for the data window see x,y,range,offset parameters*/
            ChannelMask _channels; /*!< all channels defined by the current Node ( that are allocated)*/
            
        public:
            
            ViewerInfos():Box2D(),_firstFrame(-1),_lastFrame(-1),_rgbMode(true),_displayWindow(),_channels(){}
            
            virtual ~ViewerInfos(){}
            
            void setDisplayWindow(Format format){_displayWindow=format;}
            
            const Format& getDisplayWindow() const {return _displayWindow;}
            
            void mergeDisplayWindow(const Format& other);
            
            const Box2D& getDataWindow() const {return dynamic_cast<const Box2D&>(*this);}
            
            bool operator==(const ViewerInfos& other);
            
            void operator=(const ViewerInfos &other);
            
            void firstFrame(int nb){_firstFrame=nb;}
            
            void lastFrame(int nb){_lastFrame=nb;}
            
            int firstFrame() const {return _firstFrame;}
            
            int lastFrame() const {return _lastFrame;}
            
            void setChannels(ChannelMask mask){_channels=mask;}
            
            const ChannelMask& channels() const {return _channels;}
            
            void rgbMode(bool m){_rgbMode=m;}
            
            bool rgbMode() const {return _rgbMode;}
            
            void reset();
        };
        
        class QKeyEvent;
        class QEvent;
        class QGLShaderProgram;
        class Lut;
        class InfoViewerWidget;
        class Controler;
        class Viewer;
        class ViewerTab;
        
        /**
         *@class ViewerGL
         *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
         *OpenGL related code as well as user events.
         **/
        class ViewerGL : public QGLWidget
        {
            Q_OBJECT
            
            friend class Viewer; /*!< Makes the viewer node class friend as it is inter-connected with the ViewerGL.*/
            
            /**
             *@class ZoomContext
             *@brief Holds all zoom related variables. This is an internal class used by the ViewerGL.
             **/
            class ZoomContext{
                
            public:
                
                ZoomContext():zoomX(0),zoomY(0),restToZoomX(0),restToZoomY(0),zoomFactor(1)
                {}
                
                QPointF old_zoomed_pt,old_zoomed_pt_win;
                float zoomX,zoomY;
                float restToZoomX,restToZoomY;
                float zoomFactor;
                
                void setZoomXY(float x,float y){zoomX=x;zoomY=y;}
                
                std::pair<int,int> getZoomXY() const {return std::make_pair(zoomX,zoomY);}
                
                /*the level of zoom used to display the frame*/
                void setZoomFactor(float f){zoomFactor = f;}
                
                float getZoomFactor() const {return zoomFactor;}
            };
            
            /**
             *@enum MOUSE_STATE
             *@brief basic state switching for mouse events
             **/
            enum MOUSE_STATE{DRAGGING,UNDEFINED};
            
            TextRenderer _textRenderer; /*!< The class used to render text in the viewport*/
            
            GLuint texBuffer[2]; /*!< PBO's id's used by the OpenGL context*/
            
            TextureEntry* _texID;/*!<The texture used to render data that doesn't come from the texture cache*/
            
            TextureEntry* _currentTexture;/*!<A pointer to the current texture used to display.*/
            
            GLuint texBlack[1];/*!<Id of the texture used to render a black screen when nothing is connected.*/
            
            QGLShaderProgram* shaderRGB;/*!<The shader program used to render RGB data*/
            QGLShaderProgram* shaderLC;/*!<The shader program used to render YCbCr data*/
            QGLShaderProgram* shaderBlack;/*!<The shader program used when the viewer is disconnected.*/
            
            bool _shaderLoaded;/*!<Flag to check whether the shaders have already been loaded.*/
            
            float _lut; /*!< a value coding the current color-space used to render.
                         0 = NONE , 1 = sRGB , 2 = Rec 709*/
            
            Lut* _colorSpace;/*!<The lut used to do the viewer colorspace conversion when we can't use shaders*/
            
            bool _usingColorSpace;/*!<True when the viewer is using the Lut. It locks it so
                                   the the software will not try to change the current lut at
                                   the same time.*/
            
            InfoViewerWidget* _infoViewer;/*!<Pointer to the info bar below the viewer holding pixel/mouse/format related infos*/
            
            float exposure ;/*!<current exposure setting, all pixels are multiplied
                             by pow(2,expousre) before they appear on the screen.*/
            
            ViewerInfos* _currentViewerInfos;/*!<Pointer to the ViewerInfos  used for rendering*/
            
            ViewerInfos* _blankViewerInfos;/*!<Pointer to the infos used when the viewer is disconnected.*/
            
            bool _drawing;/*!<True if the viewer is connected and not displaying black.*/
            
            MOUSE_STATE _ms;/*!<Holds the mouse state*/
            
            QPointF old_pos;/*!<The last position of the mouse*/
            
            QPointF new_pos;/*!<The new position of the mouse*/
            
            float transX,transY;/*!<The translation applied to the viewer on the
                                 X and Y coordinates.*/
            
            std::pair<int,int> _rowSpan;/*!<The first and last row index displayed currently on the viewer.*/
            
            ZoomContext _zoomCtx;/*!<All zoom related variables are packed into this object*/
            
            QString _resolutionOverlay;/*!<The string holding the resolution overlay, e.g: "1920x1080"*/
            
            QString _btmLeftBBOXoverlay;/*!<The string holding the bottom left corner coordinates of the dataWindow*/
            
            QString _topRightBBOXoverlay;/*!<The string holding the top right corner coordinates of the dataWindow**/
            
            bool _overlay;/*!<True if the user enabled overlay dispay*/
            
            bool _hasHW;/*!<True if the user has a GLSL version supporting everything requested.*/
            
            char* frameData;/*!<Pointer to the buffer holding the data computed for the last frame.
                             This buffer is either pointing to a mmaped region or to a full RAM buffer.
                             It is then copied to the PBO for rendering on the viewport.*/
            
            bool _mustFreeFrameData;/*!<True indicates that the frameData is pointing to a RAM buffer.
                                     *This is true whenever the texture displayed is just a portion of the full image(i.e
                                     *when filling the texture cache). It can also be true if there's a problem with
                                     *with the cache and indicates that the software is falling back on a pure RAM version.*/
            
            bool _noDataTransfer;/*!<True whenever the current texture already holds data and doesn't need
                                  a copy from a PBO*/
            
            TextureCache* _textureCache;/*!<Pointer to the texture cache. The viewer does NOT own it!
                                         It is shared among viewers*/
            
        public:
            
            /**
             *@enum CACHING_MODE
             *@brief This enum is used internally by the engine when it calls the function
             determineFrameDataContainer(). It indicates to the ViewerGL what kind of caching
             policy it should use and the subsequent caching process will use this policy.
             **/
            enum CACHING_MODE{TEXTURE_CACHE,VIEWER_CACHE};
            
            /*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting.
             When constructing a viewer for the 1st time in the app, you must pass a NULL shareWidget. Otherwise,you
             can pass a pointer to the 1st viewer you created. It allows the viewers to share the same OpenGL context.
             */
            ViewerGL(QWidget* parent=0, const QGLWidget* shareWidget=NULL);
            ViewerGL(const QGLFormat& format,QWidget* parent=NULL, const QGLWidget* shareWidget=NULL);
            ViewerGL(QGLContext* context,QWidget* parent=0, const QGLWidget* shareWidget=NULL);
            
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
            void drawing(bool d){_drawing=d;if(!_drawing) initBlackTex();}
            
            /**
             *@returns Returns true if the viewer is displaying something.
             **/
            bool drawing()const{return _drawing;}
            
            /**
             *@brief Convenience function.
             *Ydirection is the order of fill of the display texture:
             *either bottom to top or top to bottom.
             **/
            int Ydirection();
            
            /**
             *@brief Convenience function.
             *@returns true when we have rgba data.
             *False means it is luminance chroma
             **/
            bool rgbMode();
            
            /**
             *@returns Returns a const reference to the dataWindow of the currentFrame(BBOX)
             **/
            const Box2D& dataWindow();
            
            /**
             *@returns Returns a const reference to the displayWindow of the currentFrame(Resolution)
             **/
            const Format& displayWindow();
            
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
            float lutType() const {return _lut;}
            
            /**
             *@returns Returns the current exposure setting, all pixels are multiplied
             *by pow(2,expousre) before they appear on the screen.
             **/
            float getExposure() const {return exposure;}
            
            /**
             *@returns Returns 1.f if the viewer is using 8bit textures.
             *Returns 0.f if the viewer is using 32bit f.p textures.
             **/
            float byteMode() const;
            
            /**
             *@brief Hack to allow the resizeEvent to be publicly used elsewhere.
             *It calls QGLWidget::resizeEvent(QResizeEvent*).
             **/
            virtual void resizeEvent(QResizeEvent* event);
            
            /**
             *@returns Returns the height of the frame with the scale factor applied to it.
             **/
            float zoomedHeight(){return floor((float)displayWindow().h()*_zoomCtx.zoomFactor);}
            
            /**
             *@returns Returns the width of the frame with the scale factor applied to it.
             **/
            float zoomedWidth(){return floor((float)displayWindow().w()*_zoomCtx.zoomFactor);}
            
            /**
             *@brief Set the zoom factor used to render.
             **/
            void setZoomFactor(float f){_zoomCtx.setZoomFactor(f); emit zoomChanged(f*100);}
            
            /**
             *@returns Returns the current zoom factor that is applied to the display.
             **/
            float getZoomFactor(){return _zoomCtx.getZoomFactor();}
            
            /**
             *@brief Computes what are the rows that should be displayed on viewer
             *for the given displayWindow with the  zoom factor and  current zoom center.
             *The rows will be stored from bottom to top. The values are returned
             *as a map of displayWindow coordinates mapped to viewport cooridnates.
             *This function does not use any OpenGL function, so it can be safely called in
             *a thread that does not own the context.
             *@param ret[in,out] the map of rows that this function will fill. This is a map
             *of scan-line indexes where the key is an index in the full-res frame and the value
             *is an index in the zoomed version of the image.
             *@param displayWindow[in] The display window used to do the computations.
             *@param zoomFactor[in] The zoom factor applied to the display window.
             **/
            void computeRowSpan(std::map<int,int>& ret,const Box2D& displayWindow,float zoomFactor);
            
            /**
             *@brief Computes the viewport coordinates of the point passed in parameter.
             *This function actually does the projection to retrieve the position;
             *@param x[in] The x coordinate of the point in OpenGL coordinates.
             *@param y[in] The y coordinates of the point in OpenGL coordinates.
             *@returns Returns the viewport coordinates mapped equivalent of (x,y).
             **/
            QPoint openGLCoordToViewportCoord(int x,int y);
            
            /**
             *@brief Computes the OpenGL coordinates of the point passed in parameter.
             *This function actually does the unprojection to retrieve the position;
             *@param x[in] The x coordinate of the point in viewport coordinates.
             *@param y[in] The y coordinates of the point in viewport coordinates.
             *@returns Returns the OpenGL coordinates mapped equivalent of (x,y) as well as the depth.
             **/
            QVector3D openGLpos(int x,int y);
            
            /**
             *@brief Same as ViewerGL::openGLpos(int x,int y) but faster since it avoids a call
             *to glReadPixels(...) to retrieve the z coordinate.
             **/
            QPoint openGLpos_fast(int x,int y);
            
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
            void fitToFormat(Format displayWindow);
            
            
            /**
             *@brief Set the member _currentViewerInfos to point to the infos passed in parameter.
             *It also updates the infos displayed on the InfoViewerWidget.
             *@param viewerInfos[in] A pointer to the ViewerInfos. The ViewerGL does not take ownership of the pointer.
             *@param onInit[in] True if the this is called on initialisation of the widget. Call it always with false.
             **/
            void setCurrentViewerInfos(ViewerInfos *viewerInfos,bool onInit=false);
            
            
            /**
             *@returns Returns a pointer to the current viewer infos.
             **/
            ViewerInfos* getCurrentViewerInfos() const {return _currentViewerInfos;}
            
            /**
             *@brief Turns on the overlays on the viewer.
             **/
            void turnOnOverlay(){_overlay=true;}
            
            /**
             *@brief Turns off the overlays on the viewer.
             **/
            void turnOffOverlay(){_overlay=false;}
            
            /**
             *@brief Called by the video engine to allocate the frameData buffer depending
             *on the caching mode.
             *@param key[in] The cache key associated to the current frame. It is used to represent
             *the frame in the cache in an (almost) unique way.
             *@param w,h[in] The width and height of the scaled frame.
             *@param mode[in] This indicates what cache to use to store the frame. If mode is
             *TEXTURE_CACHE then it will allocate frameData in RAM only and copy all data in a texture.
             *If mode is VIEWER_CACHE then it will allocate frameData with a backing file on disk. The
             *buffer will then be a memory mapped portion of the address-space of the process.
             *@returns Returns the size in bytes of the space occupied in memory by the frame.
             **/
            size_t determineFrameDataContainer(U64 key,int w,int h,ViewerGL::CACHING_MODE mode);
            
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
            
            /**
             *@brief Copies the data stored in the currently mapped pbo into the currently
             *used texture. Note that it unmaps the current PBO before actually copying data
             *to the texture.
             **/
            void copyPBOtoTexture();
            
            /**
             *@returns *Returns a pointer to the data of the current frame.
             **/
            const char* getFrameData() const {return frameData;}
            
            /**
             *@returns Returns the OpenGL PBO located at index. Index must be either 0 or 1.
             **/
            GLuint getPBOId(int index) const {return texBuffer[index];}
            
            /**
             *@brief Set the _rowSpan member. This is a pair of the index of the first row and
             *last row rendered on the viewport. This is used by the paint function.
             **/
            void setRowSpan(std::pair<int,int> p){_rowSpan = p;}
            
            /**
             *@brief Set the pointer to the texture cache.
             **/
            void setTextureCache(TextureCache* cache){ _textureCache = cache;}
            
            /**
             *@returns Returns a pointer to the texture cache.
             **/
            TextureCache* getTextureCache() const {return _textureCache;}
            
            /**
             *@brief Set the current texture used to display.
             *@param texture[in] A pointer to the texture used to render.
             **/
            void setCurrentTexture(TextureEntry* texture);
            
            /**
             *@returns Returns a pointer to the current texture used to display.
             **/
            TextureEntry* getCurrentTexture() const {return _currentTexture;}
            
            /**
             *@returns Returns a pointer to the texture used to display when the texture
             *does not come frome the texture cache.
             **/
            TextureEntry* getDefaultTextureID() const {return _texID;}
            
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
             *@brief Makes the viewer display black only.
             **/
            void clearViewer();
            
            /**
             *@brief Emits signals indicating that the data window and the display have changed
             *and change the strings displayed as overlays.
             **/
            void updateDataWindowAndDisplayWindowInfo();
            
            
            /**
             *@brief set the current viewer infos to be blank(used when
             displaying black)
             **/
            void blankInfoForViewer(bool onInit=false);
            
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
             *@brief Draws the black texture. Called by paintGL()
             **/
            void drawBlackTex(); // draw the black texture
            
            /**
             *@brief Called inside paintGL(). It will draw all the overlays. It also calls
             *VideoEngine::drawOverlay()
             **/
            void drawOverlay();
            
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
             *@param w[in] The width of the scaled frame.
             *@param yOffset[in] The index of the scan-line in the scaled frame.
             **/
            void convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                            int w,int yOffset,const float* alpha);
            
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
             *@param w[in] The width of the scaled frame.
             *@param yOffset[in] The index of the scan-line in the scaled frame.
             **/
            void convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                               int w,int yOffset,const float* alpha);
            
            /**
             *@brief Set the current translation applied to the viewer
             **/
            void setTranslation(float x,float y){transX = x; transY=y;}
            
            /**
             *@brief Resets the mouse position
             **/
            void resetMousePos(){ new_pos.setX(0);new_pos.setY(0); old_pos.setX(0);old_pos.setY(0);}
            
            
            /**
             *@typedef Typedef used by all _gl functions. It defines an Eigen 4x4 matrix of floats.
             **/
            typedef Eigen::Matrix4f M44f;
            
            /**
             *@brief Computes the inverse matrix of m and stores it in out
             **/
            static bool _glInvertMatrix(float* m ,float* invOut);
            
            /**
             *@brief Multiplys matrix1 by matrix2 and stores the result in result
             **/
            static void _glMultMats44(float *result, float *matrix1, float *matrix2);
            
            /**
             *@brief Multiply the matrix by the vector and stores it in resultvector
             **/
            static void _glMultMat44Vect(float *resultvector, const float *matrix, const float *pvector);
            
            /**
             *@brief Multiplies matrix by pvector and stores only the ycomponent (multiplied by the homogenous coordinates)
             **/
            static int _glMultMat44Vect_onlyYComponent(float *yComponent, const M44f& matrix, const Eigen::Vector4f&);
            
            /**
             *@brief Replicate of the glOrtho func, but for an identity matrix.
             WARNING: All the content of matrix will be modified when returning from this function.
             **/
            static void _glOrthoFromIdentity(M44f& matrix,float left,float right,float bottom,float top,float near,float far);
            
            /**
             *@brief Replicate of the glScale function but for a custom matrix
             **/
            static void _glScale(M44f& result,const M44f& matrix,float x,float y,float z);
            
            /**
             *@brief Replicate of the glTranslate function but for a custom matrix
             **/
            static void _glTranslate(M44f& result,const M44f& matrix,float x,float y,float z);
            
            /**
             *@brief Replicate of the glRotate function but for a custom matrix.
             **/
            static void _glRotate(M44f& result,const M44f& matrix,float a,float x,float y,float z);
            
            /**
             *@briefReplicate of the glLoadIdentity function but for a custom matrix
             **/
            static void _glLoadIdentity(M44f& matrix);
            
        };
#endif // GLVIEWER_H

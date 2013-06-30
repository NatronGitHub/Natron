//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef GL_VIEWER_HEADER__
#define GL_VIEWER_HEADER__


#include "Superviser/gl_OsDependent.h" // <must be included before QGlWidget because of gl.h and glew.h
#include <cmath>
#include <QtOpenGL/QGLWidget>
#include <QtGui/QVector4D>
#include "Core/displayFormat.h"
#include "Core/channels.h"
#include "Gui/textRenderer.h"
#include "Gui/texturecache.h"

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
        
        /*Holds info necessary to render.*/
        class ViewerInfos : public Box2D{
            
            int _firstFrame;
            int _lastFrame;
            bool _rgbMode;
            Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
            ChannelMask _channels; // all channels defined by the current Node ( that are allocated)
            
        public:
            ViewerInfos():Box2D(),_firstFrame(-1),_lastFrame(-1),_rgbMode(true),_displayWindow(),_channels(){}
            
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
            
            virtual ~ViewerInfos(){}
        };
        
        class QKeyEvent;
        class QEvent;
        class QGLShaderProgram;
        class Lut;
        class InfoViewerWidget;
        class Controler;
        class Viewer;
        class ViewerTab;
        class ViewerGL : public QGLWidget
        {
            Q_OBJECT
            
            friend class Viewer;
            
            /*Class holding zoom variables*/
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
            
            
            /*basic state switching for mouse events*/
            enum MOUSE_STATE{DRAGGING,UNDEFINED};
            
            
            /*The text renderer instance*/
            TextRenderer _textRenderer;
            
            /*PBO's used to upload textures*/
            GLuint texBuffer[2];
            
            /*the texture used for rendering when the texture does not come from cache*/
            TextureEntry* _texID;
            
            /*the texture used for rendering, might come from the cache,otherwise it is texId[0]*/
            TextureEntry* _currentTexture;
            
            /*the black texture*/
            GLuint texBlack[1];
            
            /*Shaders instances*/
            QGLShaderProgram* shaderRGB;
            QGLShaderProgram* shaderLC;
            QGLShaderProgram* shaderBlack;
            
            /*true if the shaders have already been loaded*/
            bool _shaderLoaded;
            
            /*0 = NONE , 1 = sRGB , 2 = Rec 709  : this value is used by shaders*/
            float _lut;
            
            /*the lut used to do the viewer colorspace conversion when we can't use shaders*/
            Lut* _colorSpace;
            
            /*True when the viewer is using the Lut. It locks it so
             the the software will not try to change the current lut at
             the same time.*/
            bool _usingColorSpace;
            
            /*The info bar below the viewer holding pixel/mouse/format related infos*/
            InfoViewerWidget* _infoViewer;
            
            /*current exposure setting, all pixels are multiplied
             by pow(2,expousre) before they appear on the screen.*/
            float exposure ;
            
            /*The viewer infos used for rendering*/
            ViewerInfos* _currentViewerInfos;
            
            /*The viewer infos used to render the black texture.*/
            ViewerInfos* _blankViewerInfos;
            
            
            /*True if the viewer is nt disconnected*/
            bool _drawing;
            
            /*Holds the mouse state*/
            MOUSE_STATE _ms;
            
            /*The last position of the mouse*/
            QPointF old_pos;
            
            /*The new position of the mouse*/
            QPointF new_pos;
            
            /*The translation applied to the viewer on the
             X and Y coordinates.*/
            float transX,transY;
            
            /*The first and last row index displayed currently on the viewer.*/
            std::pair<int,int> _rowSpan;
            
            /*All zoom related variables are packed into this object*/
            ZoomContext _zoomCtx;
            
            /*The string holding the resolution overlay, e.g: "1920x1080"*/
            QString _resolutionOverlay;
            
            /*The string holding the bottom left corner coordinates of the dataWindow*/
            QString _btmLeftBBOXoverlay;
            
            /*The string holding the top right corner coordinates of the dataWindow*/
            QString _topRightBBOXoverlay;
            
            /*True if the user enabled overlay's dispay*/
            bool _overlay;
            
            /*True if the user has a GLSL version supporting everything requested.*/
            bool _hasHW;
            
            /*last frame computed data , either U32* or float* depending on _byteMode*/
            char* frameData;
            
            /*true whenever the texture displayed is just a portion of the full image(i.e
             when filling the texture cache).*/
            bool _mustFreeFrameData;
            
            /*true whenever the current texture already holds data and doesn't need
             a copy from a pbo*/
            bool _noDataTransfer;
            
            /*A pointer to the texture cache. The viewer does NOT own it!
             It is shared among viewers*/
            TextureCache* _textureCache;
            
            
        public:
            /*enum used by the handleTextureAndViewerCache function to determine the behaviour
             to have depending on the caching mode*/
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
            
            /*drawRow is called by the viewer node. The pointers are
             pointing to full-length buffers. They represent 1 scan-line
             at position zoomedY.*/
            void drawRow(const float* r,const float* g,const float* b,const float* a,int zoomedY);
            
            /*Print a message if the current frame buffer is incomplete.
             where will be printed to indicate a function. If silent is off,
             this function will report even when the frame buffer is complete.*/
            void checkFrameBufferCompleteness(const char where[],bool silent=true);
            
            /*initiliazes OpenGL context related stuff*/
            void initializeGL();
            
            
            /*handles the resizing of the viewer*/
            void resizeGL(int width,int height);
            
            /*drawing means we will display an image, otherwise it is black.*/
            void drawing(bool d){_drawing=d;if(!_drawing) initBlackTex();}
            bool drawing()const{return _drawing;}
            
            /*Convenience function.
             *Ydirection is the order of fill of the display texture:
             either bottom to top or top to bottom.*/
            int Ydirection();
            
            /*rgbMode is true when we have rgba data.
             False means it is luminance chroma*/
            bool rgbMode();
            
            /*The dataWindow of the currentFrame(BBOX)*/
            const Box2D& dataWindow();
            
            /*The displayWindow of the currentFrame(Resolution)*/
            const Format& displayWindow();
            
            /*handy functions to save/restore/make current the GL context*/
            void saveGLState();
            void restoreGLState();
            void makeCurrent();
            
            
            /*the lut used by the viewer to output images*/
            float lutType(){return _lut;}
            
            
            /*the exposure applied to the fragments when drawing*/
            float getExposure(){return exposure;}
            
            
            /*Returns 1.f if the viewer is using 8bit textures.
             Returns 0.f if the viewer is using 32bit f.p textures.*/
            float byteMode();
            
            /*start using the RGB shader to display the frame*/
            void activateShaderRGB();
            
            /*start using the luminance/chroma shader*/
            void activateShaderLC();
            
            
            /*overload of QT enter/leave/resize events*/
            void enterEvent(QEvent *event);
            void leaveEvent(QEvent *event);
            virtual void resizeEvent(QResizeEvent* event);
            
            /*Convenience functions to communicate with the ZoomContext*/
            float zoomedHeight(){return floor((float)displayWindow().h()*_zoomCtx.zoomFactor);}
            float zoomedWidth(){return floor((float)displayWindow().w()*_zoomCtx.zoomFactor);}
            void setZoomFactor(float f){_zoomCtx.setZoomFactor(f); emit zoomChanged(f*100);}
            float getZoomFactor(){return _zoomCtx.getZoomFactor();}
            
            /*computes what are the rows that should be displayed on viewer
             for the given displayWindow with the  zoom factor and  current zoom center.
             They will be stored from bottom to top. The values are returned
             as a map of displayWindow coordinates mapped to viewport cooridnates*/
            std::map<int,int> computeRowSpan(const Box2D& displayWindow,float zoomFactor);
            
            /*translation/zoom related functions*/
            void setTranslation(float x,float y){transX = x; transY=y;}
            std::pair<int,int> getTranslation(){return std::make_pair(transX,transY);}
            void resetMousePos(){ new_pos.setX(0);new_pos.setY(0); old_pos.setX(0);old_pos.setY(0);}
            
            
            
            /*Fill the frameData buffer with the current row . It converts
             32bit floating points intensities to 8bit using error diffusion
             algorithm. It also applies the viewer LUT during the filling process.*/
            void convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                            int w,int yOffset,const float* alpha);
            /*idem above but for floating point textures (no dithering applied here)*/
            void convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                               int w,int yOffset,const float* alpha);
            
            
            /*actually converting to ARGB... but it is called BGRA by
             the texture format GL_UNSIGNED_INT_8_8_8_8_REV*/
            static U32 toBGRA(U32 r,U32 g,U32 b,U32 a);
            
            
            /*Returns mouse position in window coordinates
             Takes in input openGL coords .*/
            QPoint mousePosFromOpenGL(int x,int y);
            
            /*returns in openGL coordinates  the mouse position
             passed in window coordinates*/
            QVector3D openGLpos(int x,int y);
            
            /*same as openGLpos(x,y) but ignores the z component,avoiding
             a call to glReadPixels()*/
            QPoint openGLpos_fast(int x,int y);
            
            /*get color of the pixel located at (x,y) in
             opengl coordinates*/
            QVector4D getColorUnderMouse(int x,int y);
            
            /*set the infoviewer pointer(the line displaying
             infos below the viewer)*/
            void setInfoViewer(InfoViewerWidget* i);
            
            /*handy function that zoom automatically the viewer so it fit
             the displayWindow  entirely in the viewer*/
            void fitToFormat(Format displayWindow);
            
            /*display black in the viewer*/
            void clearViewer();
            
            /*set the current viewer infos to be blank(used when
             displaying black)*/
            void blankInfoForViewer(bool onInit=false);
            
            /*viewerInfo related functions)*/
            void setCurrentViewerInfos(ViewerInfos *viewerInfos,bool onInit=false);
            ViewerInfos* getCurrentViewerInfos() const {return _currentViewerInfos;}
            
            /*update the BBOX info*/
            void updateDataWindowAndDisplayWindowInfo();
            
            /*turn on/off overlay*/
            void turnOnOverlay(){_overlay=true;}
            void turnOffOverlay(){_overlay=false;}
            
            
            /*called by the main thread to init specific variables per frame
             * Avoid heavy computations here.
             * That's where the viewer cached frame is initialised.
             * Returns the size in bytes of the memory allocated for the frame
             */
            size_t determineFrameDataContainer(U64 key,std::string filename,int w,int h,ViewerGL::CACHING_MODE mode);
            
            
            /*Allocate the pbo represented by the pboID with dataSize bytes.
             This function returns a pointer to
             the mapping created between the GPU and the RAM*/
            void* allocateAndMapPBO(size_t dataSize,GLuint pboID);
            
            /*Fill the mapped PBO represented by dst with byteCount of src*/
            void fillPBO(const char* src,void* dst,size_t byteCount);
            
            /*Unmap the current mapped PBO and copies it to the display texture*/
            void copyPBOtoTexture(int w,int h);
            
            void doEmitFrameChanged(int f){
                emit frameChanged(f);
            }
            
            /*Returns a pointer to the data of the current frame.*/
            const char* getFrameData(){return frameData;}
            
            GLuint getPBOId(int index){return texBuffer[index];}
            
            void setRowSpan(std::pair<int,int> p){_rowSpan = p;}
            
            void setTextureCache(TextureCache* cache){ _textureCache = cache;}
            
            TextureCache* getTextureCache(){return _textureCache;}
            
            void setCurrentTexture(TextureEntry* texture){
                if(_currentTexture)
                    _currentTexture->returnToNormalPriority();
                _currentTexture = texture;
                texture->preventFromDeletion();
            }
            
            TextureEntry* getCurrentTexture(){return _currentTexture;}
            
            TextureEntry* getDefaultTextureID(){return _texID;}
            
            bool hasHardware(){return _hasHW;}
            
            void disconnectViewer();
            
            public slots:
            virtual void updateGL();
            void updateColorSpace(QString str);
            void zoomSlot(int v);
            void zoomSlot(double v){zoomSlot((int)v);} // convenience for FeedBackSpinBox
            void zoomSlot(QString); // parse qstring and removes the % character
            void updateExposure(double);
        signals:
            void infoMousePosChanged();
            void infoColorUnderMouseChanged();
            void infoResolutionChanged();
            void infoDisplayWindowChanged();
            void zoomChanged(int v);
            void frameChanged(int);
            
            
            protected :
            /*The display function. That's where the painting occurs.*/
            virtual void paintGL();
            
            virtual void mousePressEvent(QMouseEvent *event);
            
            virtual void mouseReleaseEvent(QMouseEvent *event);
            
            virtual void mouseMoveEvent(QMouseEvent *event);
            
            virtual void wheelEvent(QWheelEvent *event);
            
        private:
            /*Called by the contructor. It avoids re-writing the same code for all constructors.*/
            void initConstructor();
            
            /*Checks extensions and init Glew for windows. Called by  initializeGL()*/
            void initAndCheckGlExtensions ();
            
            /*Initialize shaders. If they were initialized ,returns instantly.*/
            void initShaderGLSL(); // init shaders
            
            /*Returns !=0 if the extension given by its name is supported by this OpenGL version.*/
            int isExtensionSupported(const char* extension);
            
            /*Initialises the black texture, drawn when the viewer is disconnected. It allows
             the coordinate tracker on the GUI to still work even when the viewer is not connected.*/
            void initBlackTex();// init the black texture when viewer is disconnected
            
            /*Draws the black texture. Called by paintGL()*/
            void drawBlackTex(); // draw the black texture
            
            /*Called inside paintGL(). It will draw all the overlays*/
            void drawOverlay();
            
            /*computes the inverse matrix of m and stores it in out*/
            int _glInvertMatrix(float *m, float *out);
            
            /*multiply matrix1 by matrix2 and stores the result in result*/
            void _glMultMats44(float *result, float *matrix1, float *matrix2);
            
            /*multiply the matrix by the vector and stores it in resultvector*/
            void _glMultMat44Vect(float *resultvector, const float *matrix, const float *pvector);
            
            /*Multiplies matrix by pvector and stores only the ycomponent (multiplied by the homogenous coordinates)*/
            int _glMultMat44Vect_onlyYComponent(float *yComponent, const float *matrix, const float *pvector);
            
            
            
        };
#endif // GLVIEWER_H

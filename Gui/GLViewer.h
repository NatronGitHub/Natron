//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef GL_VIEWER_HEADER__
#define GL_VIEWER_HEADER__


#include "Superviser/gl_OsDependent.h" // <must be included before QGlWidget because of gl.h and glew.h

#include <QtOpenGL/QGLWidget>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <QtOpenGL/QGLFormat>
#include <ImfThreading.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaYca.h>
#include <ImfArray.h>
#include <QtOpenGL/QGLShaderProgram>
#include <QtGui/qvector4d.h>
#include "Reader/readerInfo.h"
#include <QtGui/QPainter>

#include "Core/row.h"
#include "Core/displayFormat.h"
#include "Gui/textRenderer.h"
#include "Core/viewercache.h"

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

class Lut;
class InfoViewerWidget;
class Controler;
class VideoEngine;
class ViewerGL : public QGLWidget
{
	Q_OBJECT

		/*In order to avoid the overhead of lengthy function
		calls, the diskcache and the videoEngine have direct access
		to this class. This is an exception and it is the case only
		for performance.*/
		friend class ViewerCache;
	friend class VideoEngine;

	/*basic state switching for mouse events*/
	enum MOUSE_STATE{DRAGGING,UNDEFINED};

	class ZoomContext{
	public:
		ZoomContext():zoomX(0),zoomY(0),zoomFactor(1),currentBuiltInZoom(-1),restToZoomX(0),restToZoomY(0)
		{}

		QPointF old_zoomed_pt,old_zoomed_pt_win;
		float zoomX,zoomY;
		float restToZoomX,restToZoomY;
		float zoomFactor;
		float currentBuiltInZoom;
		std::pair<int,int> zoomIncrement;

		void setZoomXY(float x,float y){zoomX=x;zoomY=y;}
		std::pair<int,int> getZoomXY(){return std::make_pair(zoomX,zoomY);}
		/*the level of zoom used to display the frame*/
		void setZoomFactor(float f){zoomFactor = f;}
		float getZoomFactor(){return zoomFactor;}
		void setCurrentBuiltInZoom(float f){currentBuiltInZoom = f;}
		float getCurrentBuiltinZoom(){return currentBuiltInZoom;}
		void setZoomIncrement(std::pair<int,int> p){zoomIncrement = p;}
	};
	class BuiltinZooms{
		std::map<float, pair<int,int> > builtInZooms;
	public:
		BuiltinZooms(){fillBuiltInZooms();}
		/*builtin factors used to compute the data: this has nothing to do
		with the level of zoom used to display*/
		void fillBuiltInZooms();
		float closestBuiltinZoom(float v);
		float inferiorBuiltinZoom(float v);
		float superiorBuiltinZoom(float v);
		std::pair<int,int>& operator[](float v){return builtInZooms[v];}

	};

public:
	/*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting*/
	ViewerGL(Controler* ctrl,float byteMode,QWidget* parent=0, const QGLWidget* shareWidget=NULL);
	ViewerGL(Controler* ctrl,float byteMode,const QGLFormat& format,QWidget* parent=NULL, const QGLWidget* shareWidget=NULL);
	ViewerGL(Controler* ctrl,float byteMode,QGLContext* context,QWidget* parent=0, const QGLWidget* shareWidget=NULL);

	virtual ~ViewerGL();

	/*set the current frame associated to the GLWidget*/
	void currentFrame(int c){_readerInfo->currentFrame(c); emit frameChanged(c);}

	/*Convenience functions
	 *set the first frame and last frame of the readerInfo associated to the GLWidget*/
	void firstFrame(int f){_readerInfo->firstFrame(f);}
	void lastFrame(int l){_readerInfo->lastFrame(l);}

	/*drawRow is called by the viewer node. Row is a pointer to the
	row to draw.*/
	void drawRow(Row *row);

	void checkFrameBufferCompleteness(const char where[],bool silent=true);

	/*initiliazes OpenGL context related stuff*/
	void initializeGL();

	/*check extensions and init Glew for windows*/
	void initAndCheckGlExtensions ();

	/*handles the resizing of the viewer*/
	void resizeGL(int width,int height);

	/*drawing means we will display an image, otherwise it is black.*/
	void drawing(bool d){_drawing=d;if(!_drawing) _firstTime=true;}
	bool drawing()const{return _drawing;}

	/*Convenience function.
	 *Ydirection is the order of fill of the display texture:
	either bottom to top or top to bottom.*/
	int Ydirection(){return _readerInfo->Ydirection();}

	/*handy functions to save/restore the GL context states*/
	void saveGLState();
	void restoreGLState();
	void makeCurrent();

	/*rgbMode is true when we have rgba data. 
	False means it is luminance chroma*/
	bool rgbMode(){return _readerInfo->rgbMode();}

	/*the lut used by the viewer to output images*/
	float lutType(){return _lut;}

	/*the builtinZoom is the level of zoom at which the data 
	have been computed, it is NOT the level of zoom currently
	set to draw the texture.*/
	/*float currentBuiltinZoom(){return currentBuiltInZoom;}*/

	/*the exposure applied to the fragments when drawing*/
	float getExposure(){return exposure;}

	/*these are the channels the viewer wants to display*/
	ChannelMask displayChannels(){return _channelsToDraw;}

	/*The dataWindow of the currentFrame(BBOX)*/
	Box2D dataWindow(){return _readerInfo->dataWindow();}

	/*The displayWindow of the currentFrame(Resolution)*/
	Format displayWindow(){return _readerInfo->displayWindow();}

	/*(ROI), not used yet*/
	Box2D& regionOfInterest(){return _roi;}
	void regionOfInterest(Box2D& renderBox){ this->_roi = renderBox;}

	/*function initialising display textures (and glsl shader the first time)
	according to the current level of zooom*/
	void initTextures();
	/*init the RGB texture*/
	void initTexturesRgb(int w,int h);
	/*width and height of the display texture*/
	std::pair<int,int> textureSize(){return _textureSize;}

	/*start using the RGB shader to display the frame*/
	void activateShaderRGB();

	/*start using the luminance/chroma shader*/
	void activateShaderLC();

	/*set the video engine pointer*/
	void setVideoEngine(VideoEngine* v){vengine = v;}

	/*overload of QT enter/leave/resize events*/
	void enterEvent(QEvent *event)
	{   QGLWidget::enterEvent(event);
        setFocus();
        grabMouse();
        grabKeyboard();
	}
	void leaveEvent(QEvent *event)
	{
		QGLWidget::leaveEvent(event);
		setFocus();
        releaseMouse();
		releaseKeyboard();
	}
	virtual void resizeEvent(QResizeEvent* event){ // public to hack the protected field
		makeCurrent();
		if(isVisible()){
			QGLWidget::resizeEvent(event);
			setVisible(true);
		}
	}

	/*zoom functions*/
	void zoomIn();
	void zoomOut();


	/*Convenience functions to communicate with the ZoomContext*/
	void setCurrentBuiltInZoom(float f){_zoomCtx.setCurrentBuiltInZoom(f);}
	float getCurrentBuiltinZoom(){return _zoomCtx.getCurrentBuiltinZoom();}
	void setZoomIncrement(std::pair<int,int> p){_zoomCtx.setZoomIncrement(p);}
	void setZoomFactor(float f){_zoomCtx.setZoomFactor(f); emit zoomChanged(f*100);}
	float getZoomFactor(){return _zoomCtx.getZoomFactor();}
	ViewerGL::BuiltinZooms& getBuiltinZooms(){return _builtInZoomMap;}

	/*translation/zoom related functions*/
	void setTranslation(float x,float y){transX = x; transY=y;}
	std::pair<int,int> getTranslation(){return std::make_pair(transX,transY);}
	void resetMousePos(){ new_pos.setX(0);new_pos.setY(0); old_pos.setX(0);old_pos.setY(0);}
	//     void setZoomXY(float x,float y){zoomX=x;zoomY=y;}
	//     std::pair<int,int> getZoomXY(){return std::make_pair(zoomX,zoomY);}

	/*the file type of the current frame*/
	void fileType(File_Type f){_filetype=f;}
	File_Type fileType(){return _filetype;}

	/*Fill the _renderingBuffer (PBO) with the current row . It converts
	32bit floating points intensities to 8bit using error diffusion
	algorithm. It also applies the viewer LUT during the filling process.
	nbBytesOutput hold the number of bytes for 1 channel in the output*/
	void convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
		size_t nbBytesOutput,int yOffset,const float* alpha=NULL);
	/*idem above but for floating point textures (no dithering applied here)*/
	void convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
		size_t nbBytesOutput,int yOffset,const float* alpha=NULL);

	/*handy functions to fill textures*/
	static U32 toBGRA(U32 r,U32 g,U32 b,U32 a);
	static QVector4D U32toBGRA(U32& c);

	/*returns mouse position in window coordinates
	Takes in input openGL coords.*/
	QPoint mousePosFromOpenGL(int x,int y);

	/*returns openGL coordinates of the mouse position
	passed in window coordinates*/
	QPoint openGLpos(int x,int y);

	/*get color of the pixel located at (x,y) in
	opengl coordinates*/
	QVector4D getColorUnderMouse(int x,int y);

	/*set the infoviewer pointer(the line displaying
	infos below the viewer)*/
	void setInfoViewer(InfoViewerWidget* i );

	/*handy function that zoom automatically the viewer so it displays
	the frame in the entirely in the viewer*/
	void initViewer();

	/*display black in the viewer*/
	void clearViewer(){
		_drawing = false;
		updateGL();
	}

	/*set the readerInfo to be blank(used when
	displaying black)*/
	void blankInfoForViewer(bool onInit=false);

	/*readerInfo related functions)*/
	void setCurrentReaderInfo(ReaderInfo* info,bool onInit=false,bool initBoundaries = false);
	void updateCurrentReaderInfo(){updateDataWindowAndDisplayWindowInfo();}
	ReaderInfo* getCurrentReaderInfo(){return _readerInfo;}

	/*the name of the current frame name*/
	QString getCurrentFrameName(){return _readerInfo->currentFrameName();}

	/*update the BBOX info*/
	void updateDataWindowAndDisplayWindowInfo();

	/*controler pointer*/
	Controler* getControler();

	/*turn on/off overlay*/
	void turnOnOverlay(){_overlay=true;}
	void turnOffOverlay(){_overlay=false;}


	/*called by the main thread to init specific variables per frame
	* safe place to do things before any computation for the frame is started. Avoid heavy computations here.
	*/
	void preProcess(std::string filename,int nbFrameHint);

	std::pair<int,int> getTextureSize(){return _textureSize;}
	std::pair<void*,size_t> allocatePBO(int w,int h);

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

	public slots:
		virtual void updateGL();
		void updateColorSpace(QString str);
		void zoomSlot(int v);
		void zoomSlot(double v){zoomSlot((int)v);} // convenience for FeedBackSpinBox
		void updateExposure(double);
signals:
		void infoMousePosChanged();
		void infoColorUnderMouseChanged();
		void infoResolutionChanged();
		void infoDisplayWindowChanged();
		void zoomChanged(int v);
		void frameChanged(int);


protected :

	virtual void paintGL();
	void drawOverlay();
	virtual void paintEvent(QPaintEvent* event);
	virtual void keyPressEvent ( QKeyEvent * event );
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

private:
	void initConstructor(); // called in the constructor
	void initShaderGLSL(); // init shaders
	int  isExtensionSupported(const char* extension); // check if OpenGL extension is supported


	void initBlackTex();// init the black texture when viewer is disconnected
	void drawBlackTex(); // draw the black texture


	TextRenderer _textRenderer;

	GLuint texBuffer[2];
	GLuint texId[1];
	//  void* _renderingBuffer; // the frame currently computed (mapped PBO handle)
	std::pair<int,int> _textureSize;
	GLuint texBlack[1];
	QGLShaderProgram* shaderRGB;
	QGLShaderProgram* shaderLC;
	QGLShaderProgram* shaderBlack;
	bool _shaderLoaded;


	float _lut; // 0 = NONE , 1 = sRGB , 2 = Rec 709  : this value is used by shaders
	Lut* _colorSpace; //the lut used to do the viewer colorspace conversion when we can't use shaders
    bool _updatingColorSpace;
    bool _usingColorSpace;

	int Ysampling; // sampling factor for LC files

	InfoViewerWidget* _infoViewer;
	Box2D _roi;  // region of interest
	float exposure ;		// Current exposure setting.  All pixels
	// are multiplied by pow(2,exposure) before
	// they appear on the screen
	ChannelMask _channelsToDraw;

	ReaderInfo* _readerInfo;
	ReaderInfo* blankReaderInfo;

	Controler* ctrl;


	bool _drawing;
	bool _firstTime;
	File_Type _filetype;

	MOUSE_STATE _ms;
	QPointF old_pos;
	QPointF new_pos;
	float transX,transY;


	ZoomContext _zoomCtx;
	BuiltinZooms _builtInZoomMap;

	VideoEngine* vengine;


	QString _resolutionOverlay;
	QString _btmLeftBBOXoverlay;
	QString _topRightBBOXoverlay;

	bool _overlay;
	float _byteMode; // boolean
	bool _hasHW;
	bool _fullscreen;

	char* frameData; // last frame computed data , either U32* or float* depending on _byteMode
	bool _makeNewFrame;
	ViewerCache::FrameID frameInfo;
    
    

};
#endif // GLVIEWER_H

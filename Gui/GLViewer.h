#ifndef SCENE_H
#define SCENE_H


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
#include "Core/diskcache.h"

using Powiter_Enums::ROW_RANK;


class Lut;
class InfoViewerWidget;
class Controler;
class VideoEngine;
class ViewerGL : public QGLWidget
{
    Q_OBJECT
    
    friend class DiskCache;
    friend class VideoEngine;
    
    enum MOUSE_STATE{DRAGGING,UNDEFINED};
public:
    ViewerGL(Controler* ctrl,float byteMode,QWidget* parent=0, const QGLWidget* shareWidget=NULL);
    ViewerGL(Controler* ctrl,float byteMode,const QGLFormat& format,QWidget* parent=NULL, const QGLWidget* shareWidget=NULL);
	ViewerGL(Controler* ctrl,float byteMode,QGLContext* context,QWidget* parent=0, const QGLWidget* shareWidget=NULL);
    virtual ~ViewerGL();
	void currentFrame(int c){_readerInfo->currentFrame(c); emit frameChanged(c);}
	void first_frame(int f){_readerInfo->firstFrame(f);}
	void last_frame(int l){_readerInfo->lastFrame(l);}
	
   
	void drawRow(Row *row,ROW_RANK rank);
	void setRow(Row* row,ROW_RANK rank){
		to_draw=row;
		drawRow(to_draw,rank);
    }
	
    
	void initializeGL();
    void resizeGL(int width,int height);
    
    void drawing(bool d){_drawing=d;if(!_drawing) _firstTime=true;}
    bool drawing()const{return _drawing;}
	int Ydirection(){return _readerInfo->Ydirection();}
	void saveGLState();
	void restoreGLState();
    void makeCurrent();
    
    bool rgbMode(){return _readerInfo->rgbMode();}
   
    float lutType(){return _lut;}
    float currentBuiltinZoom(){return currentBuiltInZoom;}
    float getExposure(){return exposure;}
    
    ChannelMask displayChannels(){return _channelsToDraw;}
    
	IntegerBox dataWindow(){return _readerInfo->dataWindow();}
	DisplayFormat displayWindow(){return _readerInfo->displayWindow();}
    IntegerBox& regionOfInterest(){return _roi;}
    void regionOfInterest(IntegerBox& renderBox){ this->_roi = renderBox;}
    void initTextures();
    std::pair<int,int> textureSize(){return _textureSize;}
    
    void activateShaderRGB();
	void activateShaderLC();
    void initAndCheckGlExtensions ();
    
    
    void setVideoEngine(VideoEngine* v){vengine = v;}
    
    void enterEvent(QEvent *event)
    {   QGLWidget::enterEvent(event);
        setFocus();
        grabKeyboard();
    }
    void leaveEvent(QEvent *event)
    {
        QGLWidget::leaveEvent(event);
        setFocus();
        releaseKeyboard();
    }
    virtual void resizeEvent(QResizeEvent* event){ // public to hack the protected field
        makeCurrent();
        if(isVisible()){
            QGLWidget::resizeEvent(event);
            setVisible(true);
        }
    }
    void zoomIn();
    void zoomOut();
    
    void setZoomFactor(float f){zoomFactor = f; emit zoomChanged(f*100);}
    float getZoomFactor(){return zoomFactor;}
    void fillBuiltInZooms();
    float closestBuiltinZoom(float v);
    float inferiorBuiltinZoom(float v);
    float superiorBuiltinZoom(float v);
    void setCurrentBuiltInZoom(float f){currentBuiltInZoom = f;}
    void setZoomIncrement(std::pair<int,int> p){ zoomIncrement = p;}
    void setTranslation(float x,float y){transX = x; transY=y;}
    std::pair<int,int> getTranslation(){return std::make_pair(transX,transY);}
    void resetMousePos(){ new_pos.setX(0);new_pos.setY(0); old_pos.setX(0);old_pos.setY(0);}
    void setZoomXY(float x,float y){zoomX=x;zoomY=y;}
    std::pair<int,int> getZoomXY(){return std::make_pair(zoomX,zoomY);}
    
	void fileType(File_Type f){_filetype=f;}
	File_Type fileType(){return _filetype;}
    
    void convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,size_t nbBytesOutput,const float* alpha=NULL);
    void convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,size_t nbBytesOutput,const float* alpha=NULL);

    
    static U32 toBGRA(U32 r,U32 g,U32 b,U32 a);
    static QVector4D U32toBGRA(U32& c);
    
    QPoint mousePosFromOpenGL(int x,int y);
    QPoint openGLpos(int x,int y);
    QVector4D getColorUnderMouse(int x,int y);

    void setInfoViewer(InfoViewerWidget* i );
    
    void initViewer();
    
    void clearViewer(){
        _drawing = false;
        updateGL();
    }
   
    void blankInfoForViewer(bool onInit=false);
    
    
    void setCurrentReaderInfo(ReaderInfo* info,bool onInit=false,bool initBoundaries = false);
    void updateCurrentReaderInfo(){updateDataWindowAndDisplayWindowInfo();}
    ReaderInfo* getCurrentReaderInfo(){return _readerInfo;}
    
    QString getCurrentFrameName(){return _readerInfo->currentFrameName();}
    
    void updateDataWindowAndDisplayWindowInfo();
    
	Controler* getControler();
    
    void turn_on_overlay(){_overlay=true;}
    void turn_off_overlay(){_overlay=false;}
    
    void initTexturesRgb();
    
    void preProcess(int nbFrameHint); // safe place to do things before any computation for the frame is started. Avoid heavy computations here.
    void postProcess(); // safe place to do things after the frame has been processed and before it is displayed
    
    void doEmitFrameChanged(int f){
        emit frameChanged(f);
    }
    
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
    void initConstructor();
    void initShaderGLSL();
    int  isExtensionSupported(const char* extension);
    
    
    void initBlackTex();
    void drawBlackTex();
    
    TextRenderer _textRenderer;
    
    GLuint texBuffer[2];
    GLuint texId[1];
    std::pair<int,int> _textureSize;
    GLuint texBlack[1];
    QGLShaderProgram* shaderRGB;
	QGLShaderProgram* shaderLC;
    QGLShaderProgram* shaderBlack;
    bool _shaderLoaded;
    
    
    float _lut; // 0 = NONE , 1 = sRGB , 2 = Rec 709  : this value is used by shaders
    Lut* _colorSpace; //the lut used to do the viewer colorspace conversion when we can't use shaders
    
    int Ysampling; // sampling factor for LC files

    InfoViewerWidget* _infoViewer;
    IntegerBox _roi;  // region of interest
	float exposure ;		// Current exposure setting.  All pixels
	// are multiplied by pow(2,exposure) before
	// they appear on the screen
    ChannelMask _channelsToDraw;
	
    ReaderInfo* _readerInfo;
    ReaderInfo* blankReaderInfo;

    Controler* ctrl;
    

	Row* to_draw;
    bool _drawing;
    bool _firstTime;
	File_Type _filetype;
    
    MOUSE_STATE _ms;
    QPointF old_pos;
    QPointF new_pos;

    QPointF old_zoomed_pt,old_zoomed_pt_win;
    float zoomX,zoomY;
    float restToZoomX,restToZoomY;
    
    float transX,transY;
    float zoomFactor;
    float currentBuiltInZoom;
    VideoEngine* vengine;
    std::map<float, pair<int,int> > builtInZooms;
    std::pair<int,int> zoomIncrement;
   
    QString _resolutionOverlay;
    QString _btmLeftBBOXoverlay;
    QString _topRightBBOXoverlay;
    
    bool _overlay;
    float _byteMode; // boolean
    bool _hasHW;
    bool _fullscreen;
    
    char* frameData; // last frame computed data , either U32* or float* depending on _byteMode
    bool _makeNewFrame;
    FrameID frameInfo;
    
};
#endif // SCENE_H
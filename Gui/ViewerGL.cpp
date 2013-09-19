//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ViewerGL.h"

#include <cassert>
#include <map>
#ifdef WITH_EIGEN
#include <Eigen/Dense>
#endif
#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QDockWidget>
#include <QtOpenGL/QGLShaderProgram>
#include <QtCore/QEvent>
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QKeyEvent>

#include "Global/Macros.h"
GCC_DIAG_OFF(unused-parameter);
#include <ImfAttribute.h> // FIXME: should be PIMPL'ed
GCC_DIAG_ON(unused-parameter);

#include "Gui/TabWidget.h"
#include "Engine/VideoEngine.h"
#include "Gui/Shaders.h"
#include "Engine/Lut.h"
#include "Global/AppManager.h"
#include "Gui/InfoViewerWidget.h"
#include "Engine/Model.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/Timeline.h"
#include "Engine/ViewerCache.h"
#include "Engine/Settings.h"
#include "Engine/MemoryFile.h"
#include "Engine/ViewerNode.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"


/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */

#ifdef __POWITER_OSX__
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#endif

using namespace Imf;
using namespace Imath;
using namespace std;
using namespace Powiter;

static const double pi= 3.14159265358979323846264338327950288419717;


static GLfloat renderingTextureCoordinates[32] = {
    0 , 1 , //0
    0 , 1 , //1
    1 , 1 ,//2
    1 , 1 , //3
    0 , 1 , //4
    0 , 1 , //5
    1 , 1 , //6
    1 , 1 , //7
    0 , 0 , //8
    0 , 0 , //9
    1 , 0 ,  //10
    1 , 0 , //11
    0 , 0 , // 12
    0 , 0 , //13
    1 , 0 , //14
    1 , 0   //15
};

/*see http://www.learnopengles.com/android-lesson-eight-an-introduction-to-index-buffer-objects-ibos/ */
static GLubyte triangleStrip[28] = {0,4,1,5,2,6,3,7,
    7,4,
    4,8,5,9,6,10,7,11,
    11,8,
    8,12,9,13,10,14,11,15
};

#ifdef WITH_EIGEN
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
static void _glOrthoFromIdentity(M44f& matrix,float left,float right,float bottom,float top,float near_,float far_);

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
#endif

/*
 ASCII art of the vertices used to render.
 The actual texture seen on the viewport is the rect (5,9,10,6).
 We draw  3*6 strips

 0 ___1___2___3
 |  /|  /|  /|
 | / | / | / |
 |/  |/  |/  |
 4---5---6----7
 |  /|  /|  /|
 | / | / | / |
 |/  |/  |/  |
 8---9--10--11
 |  /|  /|  /|
 | / | / | / |
 |/  |/  |/  |
 12--13--14--15
 */
void ViewerGL::drawRenderingVAO(){
    const TextureRect& r = _drawing ? _defaultDisplayTexture->getTextureRect() : _blackTex->getTextureRect();
    const Format& img = displayWindow();
    GLfloat vertices[32] = {
        0.f               , (GLfloat)img.height()  , //0
        (GLfloat)r.x      , (GLfloat)img.height()  , //1
        (GLfloat)r.r + 1.f, (GLfloat)img.height()  , //2
        (GLfloat)img.width()  , (GLfloat)img.height()  , //3
        0.f               , (GLfloat)r.t + 1.f, //4
        (GLfloat)r.x      , (GLfloat)r.t + 1.f, //5
        (GLfloat)r.r + 1.f, (GLfloat)r.t + 1.f, //6
        (GLfloat)img.width()  , (GLfloat)r.t + 1.f, //7
        0.f               , (GLfloat)r.y      , //8
        (GLfloat)r.x      , (GLfloat)r.y      , //9
        (GLfloat)r.r + 1.f, (GLfloat)r.y      , //10
        (GLfloat)img.width()  , (GLfloat)r.y      , //11
        0.f               , 0.f               , //12
        (GLfloat)r.x      , 0.f               , //13
        (GLfloat)r.r + 1.f, 0.f               , //14
        (GLfloat)img.width()  , 0.f                 //15
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 32*sizeof(GLfloat), vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboTexturesId);
    glClientActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0 , 0);
    
    glBindBuffer(GL_ARRAY_BUFFER,0);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboTriangleStripId);
    glDrawElements(GL_TRIANGLE_STRIP, 28, GL_UNSIGNED_BYTE, 0);
    checkGLErrors();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    checkGLErrors();
}
void ViewerGL::checkFrameBufferCompleteness(const char where[],bool silent){
	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( error == GL_FRAMEBUFFER_UNDEFINED)
		cout << where << ": Framebuffer undefined" << endl;
	else if(error == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		cout << where << ": Framebuffer incomplete attachment " << endl;
	else if(error == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
		cout << where << ": Framebuffer incomplete missing attachment" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
		cout << where << ": Framebuffer incomplete draw buffer" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
		cout << where << ": Framebuffer incomplete read buffer" << endl;
	else if( error == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE)
		cout << where << ": Framebuffer incomplete read buffer" << endl;
	else if( error== GL_FRAMEBUFFER_UNSUPPORTED)
		cout << where << ": Framebuffer unsupported" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
		cout << where << ": Framebuffer incomplete multisample" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT)
		cout << where << ": Framebuffer incomplete layer targets" << endl;
	else if(error == GL_FRAMEBUFFER_COMPLETE ){
		if(!silent)
			cout << where << ": Framebuffer complete" << endl;
	}
	else if ( error == 0)
		cout << where << ": an error occured determining the status of the framebuffer" << endl;
	else
		cout << where << ": UNDEFINED FRAMEBUFFER STATUS" << endl;
	checkGLErrors();
}

void ViewerGL::blankInfoForViewer(bool onInit){
    setCurrentViewerInfos(_blankViewerInfos,onInit);
}
void ViewerGL::initConstructor(){
    
    _hasHW=true;
    _blankViewerInfos = new ViewerInfos;
    _blankViewerInfos->set_channels(Powiter::Mask_RGBA);
    _blankViewerInfos->set_rgbMode(true);
    Format frmt(0, 0, 2048, 1556,"2K_Super_35(full-ap)",1.0);
    _blankViewerInfos->set_dataWindow(Box2D(0, 0, 2048, 1556));
    _blankViewerInfos->set_displayWindow(frmt);
    _blankViewerInfos->set_firstFrame(0);
    _blankViewerInfos->set_lastFrame(0);
    blankInfoForViewer(true);
	_drawing=false;
	exposure = 1;
	setMouseTracking(true);
	_ms = UNDEFINED;
	shaderLC=NULL;
	shaderRGB=NULL;
    shaderBlack=NULL;
    _overlay=true;
    frameData = NULL;
    _colorSpace = Color::getLut(Color::LUT_DEFAULT_VIEWER);
    _defaultDisplayTexture = 0;
    _pBOmapped = false;
    _displayChannels = 0.f;
    _progressBarY = -1;
    _drawProgressBar = false;
    _updatingTexture = false;
}

ViewerGL::ViewerGL(QGLContext* context,ViewerTab* parent,const QGLWidget* shareWidget)
: QGLWidget(context,parent,shareWidget)
, _textRenderer(this)
, _shaderLoaded(false)
, _lut(1)
, _viewerTab(parent)
, _drawing(false)
, _must_initBlackTex(true)
, _clearColor(0,0,0,255)
{ 
    initConstructor();
}

ViewerGL::ViewerGL(const QGLFormat& format,ViewerTab* parent ,const QGLWidget* shareWidget)
: QGLWidget(format,parent,shareWidget)
, _textRenderer(this)
, _shaderLoaded(false)
, _lut(1)
, _viewerTab(parent)
, _drawing(false)
, _must_initBlackTex(true)
, _clearColor(0,0,0,255)
{
    initConstructor();
}

ViewerGL::ViewerGL(ViewerTab* parent,const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _textRenderer(this)
, _shaderLoaded(false)
, _lut(1)
, _viewerTab(parent)
, _drawing(false)
, _must_initBlackTex(true)
, _clearColor(0,0,0,255)
{
    initConstructor();
}


ViewerGL::~ViewerGL(){
    if(shaderLC){
        shaderLC->removeAllShaders();
        delete shaderLC;
        
    }
    if(shaderRGB){
        shaderRGB->removeAllShaders();
        delete shaderRGB;
    }
    if(shaderBlack){
        shaderBlack->removeAllShaders();
        delete shaderBlack;
        
    }
    delete _blackTex;
    delete _defaultDisplayTexture;
    glDeleteBuffers(2, &_pboIds[0]);
    glDeleteBuffers(1, &_vboVerticesId);
    glDeleteBuffers(1, &_vboTexturesId);
    glDeleteBuffers(1, &_iboTriangleStripId);
    checkGLErrors();
    delete _blankViewerInfos;
	delete _infoViewer;
}

QSize ViewerGL::sizeHint() const{
    return QSize(500,500);
}
void ViewerGL::resizeGL(int width, int height){
    if(height == 0)// prevent division by 0
        height=1;
    float ap = displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, (int)(width*ap), height);
    }else{
        glViewport (0, 0, width, (int)(height/ap));
    }
    checkGLErrors();
    _ms = UNDEFINED;
    if(_drawing)
        emit engineNeeded();
}
void ViewerGL::paintGL()
{
    checkGLErrors();
    if (_must_initBlackTex) {
        initBlackTex();
    }
    double w = (double)width();
    double h = (double)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    assert(_zoomCtx._zoomFactor > 0);
    assert(_zoomCtx._zoomFactor <= 1024);
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / (double)_zoomCtx._zoomFactor;
    double right = left +  w / (double)_zoomCtx._zoomFactor;
    assert(left != right);
    assert(top != bottom);
    glOrtho(left, right, bottom, top, -1, 1);
    checkGLErrors();

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    
    glEnable (GL_TEXTURE_2D);
    if(_drawing){
        glBindTexture(GL_TEXTURE_2D, _defaultDisplayTexture->getTexID());
        // debug (so the OpenGL debugger can make a breakpoint here)
        // GLfloat d;
        //  glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
        if(rgbMode()) {
            activateShaderRGB();
            checkGLErrors();
        } else if(!rgbMode()) {
            activateShaderLC();
            checkGLErrors();

        }
    }else{
        glBindTexture(GL_TEXTURE_2D, _blackTex->getTexID());
        checkGLErrors();
        if(_hasHW && !shaderBlack->bind()){
            cout << qPrintable(shaderBlack->log()) << endl;
            checkGLErrors();
        }
        if(_hasHW)
            shaderBlack->setUniformValue("Tex", 0);
        checkGLErrors();
        
    }
    checkGLErrors();
    clearColorBuffer(_clearColor.redF(),_clearColor.greenF(),_clearColor.blueF(),_clearColor.alphaF());
    checkGLErrors();
    drawRenderingVAO();
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if(_drawing){
        if(_hasHW && rgbMode() && shaderRGB){
            shaderRGB->release();
        }else if (_hasHW &&  !rgbMode() && shaderLC){
            shaderLC->release();
        }
    }else{
        if(_hasHW)
            shaderBlack->release();
    }
    if(_overlay){
        drawOverlay();
    }
    if(_drawProgressBar){
        drawProgressBar();
    }
    assert_checkGLErrors();
}


void ViewerGL::clearColorBuffer(double r ,double g ,double b ,double a ){
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT);
}
void ViewerGL::backgroundColor(double &r,double &g,double &b){
    r = _clearColor.redF();
    g = _clearColor.greenF();
    b = _clearColor.blueF();
}
void ViewerGL::drawOverlay(){
    
    ///TODO: use glVertexArrays instead!
    glDisable(GL_TEXTURE_2D);
    _textRenderer.print(displayWindow().width(),0, _resolutionOverlay,QColor(233,233,233));
    
    QPoint topRight(displayWindow().width(),displayWindow().height());
    QPoint topLeft(0,displayWindow().height());
    QPoint btmLeft(0,0);
    QPoint btmRight(displayWindow().width(),0 );
    
    glBegin(GL_LINES);
    glColor4f(0.5, 0.5, 0.5,1.0);
    glVertex3f(btmRight.x(),btmRight.y(),1);
    glVertex3f(btmLeft.x(),btmLeft.y(),1);
    
    glVertex3f(btmLeft.x(),btmLeft.y(),1);
    glVertex3f(topLeft.x(),topLeft.y(),1);
    
    glVertex3f(topLeft.x(),topLeft.y(),1);
    glVertex3f(topRight.x(),topRight.y(),1);
    
    glVertex3f(topRight.x(),topRight.y(),1);
    glVertex3f(btmRight.x(),btmRight.y(),1);
    
    glEnd();
    checkGLErrors();
    if(displayWindow() != dataWindow()){
        
        _textRenderer.print(dataWindow().right(), dataWindow().top(),_topRightBBOXoverlay, QColor(150,150,150));
        _textRenderer.print(dataWindow().left(), dataWindow().bottom(), _btmLeftBBOXoverlay, QColor(150,150,150));
        
        
        QPoint topRight2(dataWindow().right(), dataWindow().top());
        QPoint topLeft2(dataWindow().left(),dataWindow().top());
        QPoint btmLeft2(dataWindow().left(),dataWindow().bottom() );
        QPoint btmRight2(dataWindow().right(),dataWindow().bottom() );
        glPushAttrib(GL_LINE_BIT);
        glLineStipple(2, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);
        glBegin(GL_LINES);
        glColor3f(0.3, 0.3, 0.3);
        glVertex3f(btmRight2.x(),btmRight2.y(),1);
        glVertex3f(btmLeft2.x(),btmLeft2.y(),1);
        
        glVertex3f(btmLeft2.x(),btmLeft2.y(),1);
        glVertex3f(topLeft2.x(),topLeft2.y(),1);
        
        glVertex3f(topLeft2.x(),topLeft2.y(),1);
        glVertex3f(topRight2.x(),topRight2.y(),1);
        
        glVertex3f(topRight2.x(),topRight2.y(),1);
        glVertex3f(btmRight2.x(),btmRight2.y(),1);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
        glPopAttrib();
        checkGLErrors();
    }
    _viewerTab->getInternalNode()->drawOverlays();
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
    checkGLErrors();
}
void ViewerGL::drawProgressBar(){
    const Format& dW = displayWindow();
    glLineWidth(5);
    glBegin(GL_LINES);
    
    glVertex3f(dW.left(),_progressBarY,1);
    glVertex3f(dW.right(),_progressBarY,1);
    
    glEnd();
    glLineWidth(1);
    checkGLErrors();
}


void ViewerGL::initializeGL(){
	initAndCheckGlExtensions();
    _blackTex = new Texture;
    _defaultDisplayTexture = new Texture;
    glGenBuffersARB(2, &_pboIds[0]);
    
    // glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &_vboVerticesId);
    glGenBuffers(1, &_vboTexturesId);
    glGenBuffers(1 , &_iboTriangleStripId);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboTexturesId);
    glBufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), renderingTextureCoordinates, GL_STATIC_DRAW);
    
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesId);
    glBufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboTriangleStripId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 28*sizeof(GLubyte), triangleStrip, GL_STATIC_DRAW);
    
    
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    checkGLErrors();
    
    
    if(_hasHW){
        shaderBlack=new QGLShaderProgram(context());
        if(!shaderBlack->addShaderFromSourceCode(QGLShader::Vertex,vertRGB))
            cout << qPrintable(shaderBlack->log()) << endl;
        if(!shaderBlack->addShaderFromSourceCode(QGLShader::Fragment,blackFrag))
            cout << qPrintable(shaderBlack->log()) << endl;
        if(!shaderBlack->link()){
            cout << qPrintable(shaderBlack->log()) << endl;
        }
        initShaderGLSL();
        checkGLErrors();
    }

    initBlackTex();
    checkGLErrors();
}

/*Little improvment to the Qt version of makeCurrent to make it faster*/
void ViewerGL::makeCurrent(){
	const QGLContext* ctx=context();
	QGLFormat viewerFormat=ctx->format();
	const QGLContext* currentCtx=QGLContext::currentContext();
	if(currentCtx){
		QGLFormat currentFormat=currentCtx->format();
		if(currentFormat == viewerFormat){
			return;
		}else{
			QGLWidget::makeCurrent();
		}
	}else{
		QGLWidget::makeCurrent();
	}
}

std::pair<int,int> ViewerGL::computeRowSpan(const Box2D& displayWindow, std::vector<int>* rows) {
    /*First off,we test the 1st and last row to check wether the
     image is contained in the viewer*/
    // testing top of the image
    assert(rows);
    assert(rows->size() == 0);
    std::pair<int,int> ret;
    int y = 0;
    int prev = -1;
    double res = toImgCoordinates_fast(0,y).y();
    ret.first = displayWindow.bottom();
    ret.second = displayWindow.top()-1;
    if (res < 0.) { // all the image is above the viewer
        return ret; // do not add any row
    }
    // testing bottom now
    y = height()-1;
    res = toImgCoordinates_fast(0,y).y();
    /*for all the others row (apart the first and last) we can check.
     */
    while(y >= 0 && res < displayWindow.bottom()){
        /*while y is an invalid line, iterate*/
        --y;
        res = toImgCoordinates_fast(0,y).y();
    }
    while(y >= 0 && res >= displayWindow.bottom() && res < displayWindow.top()){
        /*y is a valid line in widget coord && res contains the image y coord.*/
        int row = (int)std::floor(res);
        assert(row >= displayWindow.bottom() && row < displayWindow.top());
        if(row != prev){
            rows->push_back(row);
            prev = row;
        }
        --y;
        res = toImgCoordinates_fast(0,y).y();
    }
    if(rows->size() > 0){
        ret.first = rows->front();
        ret.second = rows->back();
    }
    assert(ret.first >= displayWindow.bottom() && ret.first <= std::max(displayWindow.bottom(), displayWindow.top()-1));
    assert(ret.second >= std::min(displayWindow.bottom(),displayWindow.top()-1) && ret.second < displayWindow.top());
    return ret;
}

std::pair<int,int> ViewerGL::computeColumnSpan(const Box2D& displayWindow, std::vector<int>* columns) {
    /*First off,we test the 1st and last columns to check wether the
     image is contained in the viewer*/
    // testing right of the image
    assert(columns);
    assert(columns->size() == 0);
    std::pair<int,int> ret;
    int x = width()-1;
    int prev = -1;
    double res = toImgCoordinates_fast(x,0).x();
    ret.first = displayWindow.left();
    ret.second = displayWindow.right()-1;
    if (res < 0.) { // all the image is on the left of the viewer
        _textureColumns.clear();
        return ret;
    }
    // testing right now
    x = 0;
    res = toImgCoordinates_fast(x,0).x();
    /*for all the others columns (apart the first and last) we can check.
     */
    while(x < width() && res < displayWindow.left()) {
        /*while x is an invalid column, iterate from left to right*/
        ++x;
        res = toImgCoordinates_fast(x,0).x();
    }
    while(x < width() && res >= displayWindow.left() && res < displayWindow.right()) {
        /*y is a valid column in widget coord && res contains the image x coord.*/
        int column = (int)std::floor(res);
        assert(column >= displayWindow.left() && column < displayWindow.right());
        if(column != prev){
            columns->push_back(column);
            prev = column;
        }
        ++x;
        res = toImgCoordinates_fast(x,0).x();
    }
    if(columns->size() > 0){
        ret.first = columns->front();
        ret.second = columns->back();
    }
    assert(ret.first >= displayWindow.left() && ret.first <= std::max(displayWindow.left(), displayWindow.right()-1));
    assert(ret.second >= std::min(displayWindow.left(),displayWindow.right()-1) && ret.second < displayWindow.right());
    _textureColumns = *columns;
    return ret;
}

int ViewerGL::isExtensionSupported(const char *extension){
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;
	extensions = glGetString(GL_EXTENSIONS);
	start = extensions;
	for (;;) {
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return 1;
		start = terminator;
	}
	return 0;
}
void ViewerGL::initAndCheckGlExtensions(){
	if(!QGLShaderProgram::hasOpenGLShaderPrograms()){
        cout << "Warning : GLSL not present on this hardware, no material acceleration possible." << endl;
		_hasHW = false;
	}
    
#ifdef __POWITER_WIN32__
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cout << "Cannot initialize "
        "glew: " << glewGetErrorString (err) << endl;
	}
#endif
}

void ViewerGL::activateShaderLC(){
    if(!_hasHW) return;
	if(!shaderLC->bind()){
		cout << qPrintable(shaderLC->log()) << endl;
	}
	shaderLC->setUniformValue("Tex", 0);
	shaderLC->setUniformValue("yw",1.0,1.0,1.0);
    shaderLC->setUniformValue("expMult",  (GLfloat)exposure);
    // FIXME: why a float to really represent an enum????
    shaderLC->setUniformValue("lut", (GLfloat)_lut);
    // FIXME-seeabove: why a float to really represent an enum????
    shaderLC->setUniformValue("byteMode", (GLfloat)byteMode());
    
}
void ViewerGL::activateShaderRGB(){
    if(!_hasHW) return;
	if(!shaderRGB->bind()){
		cout << qPrintable(shaderRGB->log()) << endl;
	}
    
    shaderRGB->setUniformValue("Tex", 0);
    // FIXME-seeabove: why a float to really represent an enum????
    shaderRGB->setUniformValue("byteMode", (GLfloat)byteMode());
	shaderRGB->setUniformValue("expMult",  (GLfloat)exposure);
    // FIXME-seeabove: why a float to really represent an enum????
    shaderRGB->setUniformValue("lut", (GLfloat)_lut);
    // FIXME-seeabove: why a float to really represent an enum????
    shaderRGB->setUniformValue("channels", (GLfloat)_displayChannels);
    
    
}

void ViewerGL::initShaderGLSL(){
    if(!_shaderLoaded && _hasHW){
        shaderRGB=new QGLShaderProgram(context());
        if(!shaderRGB->addShaderFromSourceCode(QGLShader::Vertex,vertRGB))
            cout << qPrintable(shaderRGB->log()) << endl;
        if(!shaderRGB->addShaderFromSourceCode(QGLShader::Fragment,fragRGB))
            cout << qPrintable(shaderRGB->log()) << endl;
        
        shaderLC = new QGLShaderProgram(context());
        if (!shaderLC->addShaderFromSourceCode(QGLShader::Vertex, vertLC)){
            cout << qPrintable(shaderLC->log()) << endl;
        }
        if(!shaderLC->addShaderFromSourceCode(QGLShader::Fragment,fragLC))
            cout << qPrintable(shaderLC->log())<< endl;
        
        
        if(!shaderRGB->link()){
            cout << qPrintable(shaderRGB->log()) << endl;
        }
        _shaderLoaded = true;
    }
    
}

void ViewerGL::saveGLState()
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
}

void ViewerGL::restoreGLState()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void ViewerGL::initBlackTex(){
    // assert(_must_initBlackTex);
    fitToFormat(displayWindow());
    
    TextureRect texSize(0, 0, 2047, 1555,2048,1556);
    assert_checkGLErrors();
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[0]);
    checkGLErrors();
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texSize.w*texSize.h*sizeof(U32), NULL, GL_DYNAMIC_DRAW_ARB);
    checkGLErrors();
    assert(!frameData);
    assert(!_pBOmapped);
    frameData = (char*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    assert(frameData);
    _pBOmapped = true;
    U32* output = reinterpret_cast<U32*>(frameData);
    for(int i = 0 ; i < texSize.w*texSize.h ; ++i) {
        output[i] = toBGRA(0, 0, 0, 255);
    }
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
    frameData = NULL;
    _pBOmapped = false;
    checkGLErrors();
    _blackTex->fillOrAllocateTexture(texSize,Texture::BYTE);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    checkGLErrors();
    _must_initBlackTex = false;
}



void ViewerGL::drawRow(const float* r,const float* g,const float* b,const float* a,int zoomedY){
    
    while(_updatingTexture){}
    if(byteMode()==0 && _hasHW){
        convertRowToFitTextureBGRA_fp(r,g,b,_textureColumns,zoomedY,a);
    }
    else{
        convertRowToFitTextureBGRA(r,g,b,_textureColumns,zoomedY,a);
        
    }
}

size_t ViewerGL::allocateFrameStorage(int w,int h){
    GLint currentBoundPBO;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    if (currentBoundPBO != 0) {
        cout << "(ViewerGL::allocateFrameStorage : Another PBO is currently mapped, glMap failed." << endl;
        return 0;
    }
    size_t dataSize = 0;
    if(byteMode() == 1 || !_hasHW){
        dataSize = sizeof(U32)*w*h;
    }else{
        dataSize = sizeof(float)*w*h*4;
    }
    assert(!frameData);
    /*MUST map the PBO AFTER that we allocate the texture.*/
    frameData = (char*)allocateAndMapPBO(dataSize,_pboIds[0]);
    assert(frameData);
    checkGLErrors();
    return dataSize;
}

void* ViewerGL::allocateAndMapPBO(size_t dataSize,GLuint pboID) {
    //cout << "    + mapping PBO" << endl;
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pboID);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
    assert(!_pBOmapped);
    GLvoid *ret = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    assert(ret);
    _pBOmapped = true;
    return ret;
}

void ViewerGL::fillPBO(const char *src, void *dst, size_t byteCount){
    assert(dst);
    assert(src);
    memcpy(dst, src, byteCount);
}

void ViewerGL::copyPBOToRenderTexture(const TextureRect& region){
    GLint currentBoundPBO;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    if (currentBoundPBO == 0) {
        cout << "(ViewerGL::copyPBOtoTexture WARNING: Attempting to copy data from a PBO that is not mapped." << endl;
        return;
    }
    /*Don't assert here. This function is not used only with the frameData buffer (see ViewerNode::cachedEngine). 
     This buffer (frameData) might be NULL if we were running in cached mode.*/
    // [FD]: where is ViewerNode::cachedEngine ?
    //assert(frameData);
    assert(_pBOmapped);
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    _pBOmapped = false;
    checkGLErrors();
    if (frameData) {
        frameData = NULL;
    }
    if(byteMode() == 1.f || !_hasHW){
        //        cout << "[COPY PBO]: " << "x = "<< region.x  << " y = " << region.y
        //        << " r = " << region.r << " t = " << region.t << " w = " << region.w
        //        << " h = " << region.h << endl;
        _defaultDisplayTexture->fillOrAllocateTexture(region,Texture::BYTE);
    }else{
        _defaultDisplayTexture->fillOrAllocateTexture(region,Texture::FLOAT);
    }
    
    // cout << "    - unmapping PBO" << endl;
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    checkGLErrors();
}

void ViewerGL::convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                          const std::vector<int>& columnSpan,int yOffset,const float* alpha){
    /*Converting one row (float32) to 8bit BGRA portion of texture. We apply a dithering algorithm based on error diffusion.
     This error diffusion will produce stripes in any image that has identical scanlines.
     To prevent this, a random horizontal position is chosen to start the error diffusion at,
     and it proceeds in both directions away from this point.*/
    assert(frameData);
    
    U32* output = reinterpret_cast<U32*>(frameData);
    unsigned int row_width = columnSpan.size();
    yOffset *= row_width;
    output += yOffset;
    
    if(_colorSpace->linear()){
        int start = (int)(rand() % row_width);
        /* go fowards from starting point to end of line: */
        for(unsigned int i = start ; i < row_width; ++i) {
            double _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            int col = columnSpan[i];
            _r = (r != NULL) ? r[col] : 0.f;
            _g = (g != NULL) ? g[col] : 0.f;
            _b = (b != NULL) ? b[col] : 0.f;
            _a = (alpha != NULL) ? alpha[col] : 1.f;

            if(!rgbMode()){
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
            }
            _r*=_a;_g*=_a;_b*=_a;
            _r*=exposure;_g*=exposure;_b*=exposure;
            a_ = (U8)std::min((int)(_a*256),255);
            r_ = (U8)std::min((int)(_r*256),255);
            g_ = (U8)std::min((int)(_g*256),255);
            b_ = (U8)std::min((int)(_b*256),255);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        /* go backwards from starting point to start of line: */
        for(int i = start-1 ; i >= 0 ; --i){
            double _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            int col = columnSpan[i];
            _r = (r != NULL) ? r[col] : 0.f;
            _g = (g != NULL) ? g[col] : 0.f;
            _b = (b != NULL) ? b[col] : 0.f;
            _a = (alpha != NULL) ? alpha[col] : 1.f;
            if(!rgbMode()){ // FIXME: what does !rgbMode() mean?
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
            }
            _r*=_a;_g*=_a;_b*=_a;
            _r*=exposure;_g*=exposure;_b*=exposure;
            a_ = (U8)std::min((int)(_a*256),255);
            r_ = (U8)std::min((int)(_r*256),255);
            g_ = (U8)std::min((int)(_g*256),255);
            b_ = (U8)std::min((int)(_b*256),255);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
    }else{ // !linear
        /*flaging that we're using the colorspace so it doesn't try to change it in the same time
         if the user requested it*/
        _usingColorSpace = true;
        int start = (int)(rand() % row_width);
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        /* go fowards from starting point to end of line: */
        //void to_float(float* to, const float* from, int W, int delta = 1) const;
        float *row_r = new float[row_width];
        float *row_g = new float[row_width];
        float *row_b = new float[row_width];
        float *row_a = new float[row_width];
        for (unsigned int i = 0 ; i < row_width; ++i) {
            int col = columnSpan[i];
            double _r,_g,_b,_a;
            _r = (r != NULL) ? r[col] : 0.f;
            _g = (g != NULL) ? g[col] : 0.f;
            _b = (b != NULL) ? b[col] : 0.f;
            _a = ((alpha != NULL) ? alpha[col] : 1.f)  * exposure;
            row_r[i] = _r;
            row_g[i] = _g;
            row_b[i] = _b;
            row_a[i] = _a;
        }
        if (!rgbMode()) { // FIXME-seeabove: what does !rgbMode() mean?
            for(unsigned int i = 0 ; i < row_width; ++i) {
                double _r,_g,_b;
                _r = row_r[i];
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
                row_r[i] = _r;
                row_g[i] = _g;
                row_b[i] = _b;
            }
        }

        float *row_r_out = new float[row_width];
        float *row_g_out = new float[row_width];
        float *row_b_out = new float[row_width];
        _colorSpace->to_float(row_r_out, row_r, row_a, row_width);
        _colorSpace->to_float(row_g_out, row_g, row_a, row_width);
        _colorSpace->to_float(row_b_out, row_b, row_a, row_width);

        for (unsigned int i = start ; i < columnSpan.size() ; ++i) {
            U8 r_,g_,b_,a_;

            error_r = (error_r&0xff) + (unsigned)row_r_out[i];
            error_g = (error_g&0xff) + (unsigned)row_g_out[i];
            error_b = (error_b&0xff) + (unsigned)row_b_out[i];
            a_ = (U8)std::min((int)((row_a[i]/exposure)*256),255);
            r_ = (U8)(error_r >> 8);
            g_ = (U8)(error_g >> 8);
            b_ = (U8)(error_b >> 8);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        /* go backwards from starting point to start of line: */
        error_r = 0x80;
        error_g = 0x80;
        error_b = 0x80;
        
        for (int i = start-1 ; i >= 0 ; --i) {
            U8 r_,g_,b_,a_;
            
            error_r = (error_r&0xff) + (unsigned)row_r_out[i];
            error_g = (error_g&0xff) + (unsigned)row_g_out[i];
            error_b = (error_b&0xff) + (unsigned)row_b_out[i];
            a_ = (U8)std::min((int)((row_a[i]/exposure)*256),255);
            r_ = (U8)(error_r >> 8);
            g_ = (U8)(error_g >> 8);
            b_ = (U8)(error_b >> 8);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        delete [] row_r;
        delete [] row_g;
        delete [] row_b;
        delete [] row_a;
        delete [] row_r_out;
        delete [] row_g_out;
        delete [] row_b_out;
        _usingColorSpace = false;
    }
    
}

// nbbytesoutput is the size in bytes of 1 channel for the row
void ViewerGL::convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                             const std::vector<int>& columnSpan,int yOffset,const float* alpha){
    assert(frameData);
    float* output = reinterpret_cast<float*>(frameData);
    // offset in the buffer : (y)*(w) where y is the zoomedY of the row and w=nbbytes/sizeof(float)*4 = nbbytes
    yOffset *= columnSpan.size()*sizeof(float);
    output+=yOffset;
    for (unsigned int i = 0 ; i < columnSpan.size(); ++i) {
        int col = columnSpan[i];
        output[i]   = (r != NULL) ? r[col] : 0.f;
        output[i+1] = (g != NULL) ? g[col] : 0.f;
        output[i+2] = (b != NULL) ? b[col] : 0.f;
        output[i+3] = (alpha != NULL) ? alpha[col] : 1.f;
    }
    
}


U32 ViewerGL::toBGRA(U32 r,U32 g,U32 b,U32 a){
    U32 res = 0x0;
    res |= b;
    res |= (g << 8);
    res |= (r << 16);
    res |= (a << 24);
    return res;
    
}

void ViewerGL::mousePressEvent(QMouseEvent *event){
    _zoomCtx._oldClick = event->pos();
    if(event->button() != Qt::MiddleButton ){
        if(!_viewerTab->getInternalNode()->notifyOverlaysPenDown(event->posF(),
                                                                 toImgCoordinates_fast(event->x(), event->y()))){
            
            _ms = DRAGGING;
        }
    }
    QGLWidget::mousePressEvent(event);
}
void ViewerGL::mouseReleaseEvent(QMouseEvent *event){
    _ms = UNDEFINED;
    _viewerTab->getInternalNode()->notifyOverlaysPenUp(event->posF(),
                                                         toImgCoordinates_fast(event->x(), event->y()));
    QGLWidget::mouseReleaseEvent(event);
}
void ViewerGL::mouseMoveEvent(QMouseEvent *event){
    QPointF pos = toImgCoordinates_fast(event->x(), event->y());
    
    if(!_viewerTab->getInternalNode()->notifyOverlaysPenMotion(event->posF(),pos)){
        
        const Format& dispW = displayWindow();
        if(pos.x() >= dispW.left() &&
           pos.x() <= dispW.width() &&
           pos.y() >= dispW.bottom() &&
           pos.y() <= dispW.height() &&
           event->x() >= 0 && event->x() < width() &&
           event->y() >= 0 && event->y() < height()){
            if(!_infoViewer->colorAndMouseVisible()){
                _infoViewer->showColorAndMouseInfo();
            }
            VideoEngine* videoEngine = _viewerTab->getInternalNode()->getVideoEngine();
            if(videoEngine && !videoEngine->isWorking()){
                updateColorPicker(event->x(),event->y());
            }
            _infoViewer->setMousePos(QPoint((int)pos.x(),(int)pos.y()));
            emit infoMousePosChanged();
            
            
            
        }else{
            if(_infoViewer->colorAndMouseVisible()){
                _infoViewer->hideColorAndMouseInfo();
            }
        }
        
        
        if(_ms == DRAGGING){
            // if(!ctrlPTR->getModel()->getVideoEngine()->isWorking() || !_drawing){
            QPoint newClick =  event->pos();
            QPointF newClick_opengl = toImgCoordinates_fast(newClick.x(),newClick.y());
            QPointF oldClick_opengl = toImgCoordinates_fast(_zoomCtx._oldClick.x(),_zoomCtx._oldClick.y());
            float dy = (oldClick_opengl.y() - newClick_opengl.y());
            _zoomCtx._bottom += dy;
            _zoomCtx._left += (oldClick_opengl.x() - newClick_opengl.x());
            _zoomCtx._oldClick = newClick;
            if(_drawing){
                emit engineNeeded();
            }
            //    else
            updateGL();
            //  }
            
        }
    }
}
void ViewerGL::updateColorPicker(int x,int y){
    QPoint pos;
    bool xInitialized = false;
    bool yInitialized = false;
    if(x != INT_MAX){
        xInitialized = true;
        pos.setX(x);
    }
    if(y != INT_MAX){
        yInitialized = true;
        pos.setY(y);
    }
    QPoint currentPos = mapFromGlobal(QCursor::pos());
    if(!xInitialized){
        pos.setX(currentPos.x());
    }
    if(!yInitialized){
        pos.setY(currentPos.y());
    }
    QVector4D color = getColorUnderMouse(pos.x(),pos.y());
    //   cout << "r: " << color.x() << " g: " << color.y() << " b: " << color.z() << endl;
    _infoViewer->setColor(color);
    emit infoColorUnderMouseChanged();
}

void ViewerGL::wheelEvent(QWheelEvent *event) {
    // if(!ctrlPTR->getModel()->getVideoEngine()->isWorking() || !_drawing){
    
    double newZoomFactor;
    if(event->delta() >0) {
        newZoomFactor =   _zoomCtx._zoomFactor*std::pow(1.01,event->delta());
    } else {
        newZoomFactor = _zoomCtx._zoomFactor/std::pow(1.01,-event->delta());
    }
    if(newZoomFactor <= 0.01) {
        newZoomFactor = 0.01;
    } else if(newZoomFactor > 1024.) {
        newZoomFactor = 1024.;
    }
    QPointF zoomCenter = toImgCoordinates_fast(event->x(), event->y());
    double zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
    _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio;
    _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    if(_drawing){
        _viewerTab->getGui()->getApp()->clearPlaybackCache();
        emit engineNeeded();
    }
    updateGL();
    
    assert(0 < _zoomCtx._zoomFactor && _zoomCtx._zoomFactor <= 1024);
    int zoomValue = (int)(100*_zoomCtx._zoomFactor);
    if (zoomValue == 0) {
        zoomValue = 1; // sometimes, floor(100*0.01) makes 0
    }
    assert(zoomValue > 0);
    emit zoomChanged(zoomValue);
}
void ViewerGL::setZoomFactor(double f){
    assert(f>0. && f <= 1024);
    _zoomCtx.setZoomFactor(f);
    emit zoomChanged((int)(f*100));
}

void ViewerGL::zoomSlot(int v){
    assert(v > 0);
    if(!_viewerTab->getGui()->getApp()->getVideoEngine()->isWorking()){
        double value = v/100.f;
        if(value < 0.01) {
            value = 0.01;
        } else if (value > 1024.) {
            value = 1024.;
        }
        _zoomCtx._zoomFactor = value;
        if(_drawing){
            _viewerTab->getGui()->getApp()->clearPlaybackCache();
            emit engineNeeded();
        }else{
            updateGL();
        }
    }
}
void ViewerGL::zoomSlot(QString str){
    str.remove(QChar('%'));
    int v = str.toInt();
    assert(v > 0);
    zoomSlot(v);
}

QPoint ViewerGL::toWidgetCoordinates(int x, int y){
    double w = width() ;
    double h = height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPoint((int)(((x - left)/(right - left))*w),(int)(((y - top)/(bottom - top))*h));
}
/*Returns coordinates with 0,0 at top left, Powiter inverts
 y as such : y= displayWindow().height() - y  to get the coordinates
 with 0,0 at bottom left*/
QVector3D ViewerGL::toImgCoordinates_slow(int x,int y){
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX=0, winY=0, winZ=0;
    GLdouble posX=0, posY=0, posZ=0;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    winX = (float)x;
    winY = viewport[3]- y;
    glReadPixels( x, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    checkGLErrors();
    return QVector3D(posX,posY,posZ);
}

QVector4D ViewerGL::getColorUnderMouse(int x,int y){
    
    QPointF pos = toImgCoordinates_fast(x, y);
    if(pos.x() < displayWindow().left() || pos.x() >= displayWindow().width() || pos.y() < displayWindow().bottom() || pos.y() >=displayWindow().height())
        return QVector4D(0,0,0,0);
    if(byteMode()==1 || !_hasHW){
        U32 pixel;
        glReadBuffer(GL_FRONT);
        glReadPixels( x, height()-y, 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 r=0,g=0,b=0,a=0;
        b |= pixel;
        g |= (pixel >> 8);
        r |= (pixel >> 16);
        a |= (pixel >> 24);
        checkGLErrors();
        return QVector4D((float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f);//U32toBGRA(pixel);
    }else if(byteMode()==0 && _hasHW){
        GLfloat pixel[4];
        glReadPixels( x, height()-y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        checkGLErrors();
        return QVector4D(pixel[0],pixel[1],pixel[2],pixel[3]);
    }
    return QVector4D(0,0,0,0);
}

void ViewerGL::fitToFormat(Format displayWindow){
    double h = displayWindow.height();
    double w = displayWindow.width();
    assert(h > 0. && w > 0.);
    double zoomFactor = height()/h;
    zoomFactor = (zoomFactor > 0.06) ? (zoomFactor-0.05) : std::max(zoomFactor,0.01);
    assert(zoomFactor>=0.01 && zoomFactor <= 1024);
    setZoomFactor(zoomFactor);
    resetMousePos();
    _zoomCtx._left = w/2.f - (width()/(2.f*_zoomCtx._zoomFactor));
    _zoomCtx._bottom = h/2.f - (height()/(2.f*_zoomCtx._zoomFactor));
    
}



void ViewerGL::setInfoViewer(InfoViewerWidget* i ){
    
    _infoViewer = i;
    QObject::connect(this,SIGNAL(infoMousePosChanged()), _infoViewer, SLOT(updateCoordMouse()));
    QObject::connect(this,SIGNAL(infoColorUnderMouseChanged()),_infoViewer,SLOT(updateColor()));
    QObject::connect(this,SIGNAL(infoResolutionChanged()),_infoViewer,SLOT(changeResolution()));
    QObject::connect(this,SIGNAL(infoDataWindowChanged()),_infoViewer,SLOT(changeDataWindow()));
    
    
}
void ViewerGL::setCurrentViewerInfos(ViewerInfos* viewerInfos,bool){
    _currentViewerInfos = viewerInfos;
    Format* df = appPTR->findExistingFormat(displayWindow().width(), displayWindow().height());

    if(df)
        _currentViewerInfos->displayWindow().name(df->name());
    updateDataWindowAndDisplayWindowInfo();
}

void ViewerGL::updateDataWindowAndDisplayWindowInfo(){
    emit infoResolutionChanged();
    emit infoDataWindowChanged();
    _resolutionOverlay.clear();
    _resolutionOverlay.append(QString::number(displayWindow().width()));
    _resolutionOverlay.append("x");
    _resolutionOverlay.append(QString::number(displayWindow().height()));
    _btmLeftBBOXoverlay.clear();
    _btmLeftBBOXoverlay.append(QString::number(dataWindow().left()));
    _btmLeftBBOXoverlay.append(",");
    _btmLeftBBOXoverlay.append(QString::number(dataWindow().bottom()));
    _topRightBBOXoverlay.clear();
    _topRightBBOXoverlay.append(QString::number(dataWindow().right()));
    _topRightBBOXoverlay.append(",");
    _topRightBBOXoverlay.append(QString::number(dataWindow().top()));
    
    
    
}
void ViewerGL::updateColorSpace(QString str){
    while(_usingColorSpace){}
    if (str == "Linear(None)") {
        if(_lut != 0){ // if it wasnt already this setting
            _colorSpace = Color::getLut(Color::LUT_DEFAULT_FLOAT);
        }
        _lut = 0;
    }else if(str == "sRGB"){
        if(_lut != 1){ // if it wasnt already this setting
            _colorSpace = Color::getLut(Color::LUT_DEFAULT_VIEWER);
        }
        
        _lut = 1;
    }else if(str == "Rec.709"){
        if(_lut != 2){ // if it wasnt already this setting
            _colorSpace = Color::getLut(Color::LUT_DEFAULT_MONITOR);
        }
        _lut = 2;
    }
    if (!_drawing) {
        return;
    }
    
    if(byteMode()==1 || !_hasHW)
        emit engineNeeded();
    else
        updateGL();
    
}
void ViewerGL::updateExposure(double d){
    exposure = d;
    if((byteMode()==1 || !_hasHW) && _drawing)
        emit engineNeeded();
    else
        updateGL();
    
}

#ifdef WITH_EIGEN
bool glInvertMatrix(float *m ,float* invOut){
    double inv[16], det;
    int i;
    
    inv[0] = m[5]  * m[10] * m[15] -
    m[5]  * m[11] * m[14] -
    m[9]  * m[6]  * m[15] +
    m[9]  * m[7]  * m[14] +
    m[13] * m[6]  * m[11] -
    m[13] * m[7]  * m[10];
    
    inv[4] = -m[4]  * m[10] * m[15] +
    m[4]  * m[11] * m[14] +
    m[8]  * m[6]  * m[15] -
    m[8]  * m[7]  * m[14] -
    m[12] * m[6]  * m[11] +
    m[12] * m[7]  * m[10];
    
    inv[8] = m[4]  * m[9] * m[15] -
    m[4]  * m[11] * m[13] -
    m[8]  * m[5] * m[15] +
    m[8]  * m[7] * m[13] +
    m[12] * m[5] * m[11] -
    m[12] * m[7] * m[9];
    
    inv[12] = -m[4]  * m[9] * m[14] +
    m[4]  * m[10] * m[13] +
    m[8]  * m[5] * m[14] -
    m[8]  * m[6] * m[13] -
    m[12] * m[5] * m[10] +
    m[12] * m[6] * m[9];
    
    inv[1] = -m[1]  * m[10] * m[15] +
    m[1]  * m[11] * m[14] +
    m[9]  * m[2] * m[15] -
    m[9]  * m[3] * m[14] -
    m[13] * m[2] * m[11] +
    m[13] * m[3] * m[10];
    
    inv[5] = m[0]  * m[10] * m[15] -
    m[0]  * m[11] * m[14] -
    m[8]  * m[2] * m[15] +
    m[8]  * m[3] * m[14] +
    m[12] * m[2] * m[11] -
    m[12] * m[3] * m[10];
    
    inv[9] = -m[0]  * m[9] * m[15] +
    m[0]  * m[11] * m[13] +
    m[8]  * m[1] * m[15] -
    m[8]  * m[3] * m[13] -
    m[12] * m[1] * m[11] +
    m[12] * m[3] * m[9];
    
    inv[13] = m[0]  * m[9] * m[14] -
    m[0]  * m[10] * m[13] -
    m[8]  * m[1] * m[14] +
    m[8]  * m[2] * m[13] +
    m[12] * m[1] * m[10] -
    m[12] * m[2] * m[9];
    
    inv[2] = m[1]  * m[6] * m[15] -
    m[1]  * m[7] * m[14] -
    m[5]  * m[2] * m[15] +
    m[5]  * m[3] * m[14] +
    m[13] * m[2] * m[7] -
    m[13] * m[3] * m[6];
    
    inv[6] = -m[0]  * m[6] * m[15] +
    m[0]  * m[7] * m[14] +
    m[4]  * m[2] * m[15] -
    m[4]  * m[3] * m[14] -
    m[12] * m[2] * m[7] +
    m[12] * m[3] * m[6];
    
    inv[10] = m[0]  * m[5] * m[15] -
    m[0]  * m[7] * m[13] -
    m[4]  * m[1] * m[15] +
    m[4]  * m[3] * m[13] +
    m[12] * m[1] * m[7] -
    m[12] * m[3] * m[5];
    
    inv[14] = -m[0]  * m[5] * m[14] +
    m[0]  * m[6] * m[13] +
    m[4]  * m[1] * m[14] -
    m[4]  * m[2] * m[13] -
    m[12] * m[1] * m[6] +
    m[12] * m[2] * m[5];
    
    inv[3] = -m[1] * m[6] * m[11] +
    m[1] * m[7] * m[10] +
    m[5] * m[2] * m[11] -
    m[5] * m[3] * m[10] -
    m[9] * m[2] * m[7] +
    m[9] * m[3] * m[6];
    
    inv[7] = m[0] * m[6] * m[11] -
    m[0] * m[7] * m[10] -
    m[4] * m[2] * m[11] +
    m[4] * m[3] * m[10] +
    m[8] * m[2] * m[7] -
    m[8] * m[3] * m[6];
    
    inv[11] = -m[0] * m[5] * m[11] +
    m[0] * m[7] * m[9] +
    m[4] * m[1] * m[11] -
    m[4] * m[3] * m[9] -
    m[8] * m[1] * m[7] +
    m[8] * m[3] * m[5];
    
    inv[15] = m[0] * m[5] * m[10] -
    m[0] * m[6] * m[9] -
    m[4] * m[1] * m[10] +
    m[4] * m[2] * m[9] +
    m[8] * m[1] * m[6] -
    m[8] * m[2] * m[5];
    
    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    
    if (det == 0)
        return false;
    
    det = 1.0 / det;
    
    for (i = 0; i < 16; ++i)
        invOut[i] = inv[i] * det;
    
    return true;
}
void glMultMats44(float *result, float *matrix1, float *matrix2){
    result[0]=matrix1[0]*matrix2[0]+
    matrix1[4]*matrix2[1]+
    matrix1[8]*matrix2[2]+
    matrix1[12]*matrix2[3];
    result[4]=matrix1[0]*matrix2[4]+
    matrix1[4]*matrix2[5]+
    matrix1[8]*matrix2[6]+
    matrix1[12]*matrix2[7];
    result[8]=matrix1[0]*matrix2[8]+
    matrix1[4]*matrix2[9]+
    matrix1[8]*matrix2[10]+
    matrix1[12]*matrix2[11];
    result[12]=matrix1[0]*matrix2[12]+
    matrix1[4]*matrix2[13]+
    matrix1[8]*matrix2[14]+
    matrix1[12]*matrix2[15];
    result[1]=matrix1[1]*matrix2[0]+
    matrix1[5]*matrix2[1]+
    matrix1[9]*matrix2[2]+
    matrix1[13]*matrix2[3];
    result[5]=matrix1[1]*matrix2[4]+
    matrix1[5]*matrix2[5]+
    matrix1[9]*matrix2[6]+
    matrix1[13]*matrix2[7];
    result[9]=matrix1[1]*matrix2[8]+
    matrix1[5]*matrix2[9]+
    matrix1[9]*matrix2[10]+
    matrix1[13]*matrix2[11];
    result[13]=matrix1[1]*matrix2[12]+
    matrix1[5]*matrix2[13]+
    matrix1[9]*matrix2[14]+
    matrix1[13]*matrix2[15];
    result[2]=matrix1[2]*matrix2[0]+
    matrix1[6]*matrix2[1]+
    matrix1[10]*matrix2[2]+
    matrix1[14]*matrix2[3];
    result[6]=matrix1[2]*matrix2[4]+
    matrix1[6]*matrix2[5]+
    matrix1[10]*matrix2[6]+
    matrix1[14]*matrix2[7];
    result[10]=matrix1[2]*matrix2[8]+
    matrix1[6]*matrix2[9]+
    matrix1[10]*matrix2[10]+
    matrix1[14]*matrix2[11];
    result[14]=matrix1[2]*matrix2[12]+
    matrix1[6]*matrix2[13]+
    matrix1[10]*matrix2[14]+
    matrix1[14]*matrix2[15];
    result[3]=matrix1[3]*matrix2[0]+
    matrix1[7]*matrix2[1]+
    matrix1[11]*matrix2[2]+
    matrix1[15]*matrix2[3];
    result[7]=matrix1[3]*matrix2[4]+
    matrix1[7]*matrix2[5]+
    matrix1[11]*matrix2[6]+
    matrix1[15]*matrix2[7];
    result[11]=matrix1[3]*matrix2[8]+
    matrix1[7]*matrix2[9]+
    matrix1[11]*matrix2[10]+
    matrix1[15]*matrix2[11];
    result[15]=matrix1[3]*matrix2[12]+
    matrix1[7]*matrix2[13]+
    matrix1[11]*matrix2[14]+
    matrix1[15]*matrix2[15];
}
void glMultMat44Vect(float *resultvector, const float *matrix, const float *pvector){
    resultvector[0]=matrix[0]*pvector[0]+matrix[4]*pvector[1]+matrix[8]*pvector[2]+matrix[12]*pvector[3];
    resultvector[1]=matrix[1]*pvector[0]+matrix[5]*pvector[1]+matrix[9]*pvector[2]+matrix[13]*pvector[3];
    resultvector[2]=matrix[2]*pvector[0]+matrix[6]*pvector[1]+matrix[10]*pvector[2]+matrix[14]*pvector[3];
    resultvector[3]=matrix[3]*pvector[0]+matrix[7]*pvector[1]+matrix[11]*pvector[2]+matrix[15]*pvector[3];
}
int glMultMat44Vect_onlyYComponent(float *yComponent,const M44f& matrix, const Eigen::Vector4f& pvector){
    Eigen::Vector4f ret = matrix * pvector;
    if(!ret.w()) return 0;
    ret.w() = 1.f / ret.w();
    *yComponent =  ret.y() * ret.w();
    return 1;
}

/*Replicate of the glOrtho func, but for a custom matrix*/
void glOrthoFromIdentity(M44f& matrix,float left,float right,float bottom,float top,float near_,float far_){
    float r_l = right - left;
    float t_b = top - bottom;
    float f_n = far_ - near_;
    float tx = - (right + left) / (right - left);
    float ty = - (top + bottom) / (top - bottom);
    float tz = - (far_ + near_) / (far_ - near_);
    matrix(0,0) = 2.0f / r_l;
    matrix(0,1) = 0.0f;
    matrix(0,2) = 0.0f;
    matrix(0,3) = tx;
    
    matrix(1,0) = 0.0f;
    matrix(1,1) = 2.0f / t_b;
    matrix(1,2) = 0.0f;
    matrix(1,3) = ty;
    
    matrix(2,0) = 0.0f;
    matrix(2,1) = 0.0f;
    matrix(2,2) = 2.0f / f_n;
    matrix(2,3) = tz;
    
    matrix(3,0) = 0.0f;
    matrix(3,1) = 0.0f;
    matrix(3,2) = 0.0f;
    matrix(3,3) = 1.0f;
}

/*Replicate of the glScale function but for a custom matrix*/
void glScale(M44f& result,const M44f& matrix,float x,float y,float z){
    M44f scale;
    scale(0,0) = x;
    scale(0,1) = 0;
    scale(0,2) = 0;
    scale(0,3) = 0;
    
    scale(1,0) = 0;
    scale(1,1) = y;
    scale(1,2) = 0;
    scale(1,3) = 0;
    
    scale(2,0) = 0;
    scale(2,1) = 0;
    scale(2,2) = z;
    scale(2,3) = 0;
    
    scale(3,0) = 0;
    scale(3,1) = 0;
    scale(3,2) = 0;
    scale(3,3) = 1;
    
    result = matrix * scale;
}

/*Replicate of the glTranslate function but for a custom matrix*/
void glTranslate(M44f& result,const M44f& matrix,float x,float y,float z){
    M44f translate;
    translate(0,0) = 1;
    translate(0,1) = 0;
    translate(0,2) = 0;
    translate(0,3) = x;
    
    translate(1,0) = 0;
    translate(1,1) = 1;
    translate(1,2) = 0;
    translate(1,3) = y;
    
    translate(2,0) = 0;
    translate(2,1) = 0;
    translate(2,2) = 1;
    translate(2,3) = z;
    
    translate(3,0) = 0;
    translate(3,1) = 0;
    translate(3,2) = 0;
    translate(3,3) = 1;
    
    result = matrix * translate;
}

/*Replicate of the glRotate function but for a custom matrix*/
void glRotate(M44f& result,const M44f& matrix,float a,float x,float y,float z){
    
    a = a * pi / 180.0; // convert to radians
	float s = sin(a);
	float c = cos(a);
	float t = 1.0 - c;
    
	float tx = t * x;
	float ty = t * y;
	float tz = t * z;
	
	float sz = s * z;
	float sy = s * y;
	float sx = s * x;
    
	M44f rotate;
	rotate(0,0)  = tx * x + c;
	rotate(0,1)   = tx * y + sz;
	rotate(0,2)   = tx * z - sy;
	rotate(0,3)   = 0;
    
	rotate(1,0)   = tx * y - sz;
	rotate(1,1)   = ty * y + c;
	rotate(1,2)   = ty * z + sx;
	rotate(1,3)   = 0;
    
	rotate(2,0)   = tx * z + sy;
	rotate(2,1)   = ty * z - sx;
	rotate(2,2)  = tz * z + c;
	rotate(2,3)  = 0;
    
	rotate(3,0)  = 0;
	rotate(3,1)  = 0;
	rotate(3,2)  = 0;
	rotate(3,3)  = 1;
    
    result = matrix * rotate;
    
}

/*Replicate of the glLoadIdentity function but for a custom matrix*/
void glLoadIdentity(M44f& matrix){
    
    matrix.setIdentity();
}
#endif // WITH_EIGEN

void ViewerGL::disconnectViewer(){
    _viewerTab->getInternalNode()->getVideoEngine()->abort(); // aborting current work
    blankInfoForViewer();
    fitToFormat(displayWindow());
    clearViewer();
}

/*rgbMode is true when we have rgba data.
 False means it is luminance chroma*/
bool ViewerGL::rgbMode(){return getCurrentViewerInfos()->rgbMode();}

/*The dataWindow of the currentFrame(BBOX)*/
const Box2D& ViewerGL::dataWindow(){return getCurrentViewerInfos()->dataWindow();}

/*The displayWindow of the currentFrame(Resolution)*/
const Format& ViewerGL::displayWindow(){return getCurrentViewerInfos()->displayWindow();}

/*display black in the viewer*/
void ViewerGL::clearViewer(){
    drawing(false);
    updateGL();
}

/*overload of QT enter/leave/resize events*/
void ViewerGL::enterEvent(QEvent *event)
{   QGLWidget::enterEvent(event);
    setFocus();
    _viewerTab->getInternalNode()->notifyOverlaysFocusGained();

}
void ViewerGL::leaveEvent(QEvent *event)
{
    QGLWidget::leaveEvent(event);
    _viewerTab->getInternalNode()->notifyOverlaysFocusLost();

}
void ViewerGL::resizeEvent(QResizeEvent* event){ // public to hack the protected field
                                                 // if(isVisible()){
    QGLWidget::resizeEvent(event);
    // }
    
}

void ViewerGL::keyPressEvent(QKeyEvent* event){
    if(event->isAutoRepeat())
        _viewerTab->getInternalNode()->notifyOverlaysKeyRepeat(event);
    else{
        _viewerTab->getInternalNode()->notifyOverlaysKeyDown(event);
    }
}


void ViewerGL::keyReleaseEvent(QKeyEvent* event){
    _viewerTab->getInternalNode()->notifyOverlaysKeyUp(event);
}


float ViewerGL::byteMode() const {
    return Settings::getPowiterCurrentSettings()->_viewerSettings.byte_mode;
}

void ViewerGL::setDisplayChannel(const ChannelSet& channels,bool yMode){
    if(yMode){
        _displayChannels = 5.f;
        
    }else{
        if(channels == Powiter::Mask_RGB || channels == Powiter::Mask_RGBA)
            _displayChannels = 0.f;
        else if((channels & Powiter::Channel_red) == Powiter::Channel_red)
            _displayChannels = 1.f;
        else if((channels & Powiter::Channel_green) == Powiter::Channel_green)
            _displayChannels = 2.f;
        else if((channels & Powiter::Channel_blue) == Powiter::Channel_blue)
            _displayChannels = 3.f;
        else if((channels & Powiter::Channel_alpha) == Powiter::Channel_alpha)
            _displayChannels = 4.f;
        
    }
    updateGL();
    
}
void ViewerGL::updateProgressOnViewer(const TextureRect& region,int y , int texY) {
    _updatingTexture = true;
    assert(frameData);
    assert(_pBOmapped);
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    checkGLErrors();
    _pBOmapped = false;
    frameData = NULL;
    if(byteMode() == 1.f || !_hasHW){
        _defaultDisplayTexture->updatePartOfTexture(region,texY,Texture::BYTE);
    }else{
        _defaultDisplayTexture->updatePartOfTexture(region,texY,Texture::FLOAT);
    }
    _drawProgressBar = true;
    _progressBarY = y;
    updateGL();
    assert(!frameData);
    frameData = (char*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    _pBOmapped = true;

    _updatingTexture = false;
}

void ViewerGL::doSwapBuffers(){
    swapBuffers();
}

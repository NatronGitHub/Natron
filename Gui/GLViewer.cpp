//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */
#include "Gui/GLViewer.h"
#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QtGui/QFont>
#include <QtWidgets/QDockWidget>
#include <QtOpenGL/QGLShaderProgram>
#include <cmath>
#include <cassert>
#include <map>
#include "Gui/tabwidget.h"
#include "Core/VideoEngine.h"
#include "Gui/shaders.h"
#include "Core/lookUpTables.h"
#include "Superviser/controler.h"
#include "Gui/InfoViewerWidget.h"
#include "Core/model.h"
#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/FeedbackSpinBox.h"
#include "Core/lutclasses.h"
#include "Gui/timeline.h"
#include "Gui/texturecache.h"
#include "Core/viewercache.h"
#include "Core/settings.h"
#include "Core/row.h"
#include "Core/viewerNode.h"
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include "Core/mappedfile.h"
#include "Superviser/powiterFn.h"

using namespace Imf;
using namespace Imath;
using namespace std;

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
    _blankViewerInfos->setChannels(Mask_RGBA);
    _blankViewerInfos->setYdirection(1);
    _blankViewerInfos->rgbMode(true);
    Format frmt(0, 0, 2048, 1556,"2K_Super_35(full-ap)",1.0);
    _blankViewerInfos->set(0, 0, 2048, 1556);
    _blankViewerInfos->setDisplayWindow(frmt);
    _blankViewerInfos->firstFrame(0);
    _blankViewerInfos->lastFrame(0);
    blankInfoForViewer(true);
	_drawing=false;
	exposure = 1;
	setMouseTracking(true);
	_ms = UNDEFINED;
    _firstTime = true;
	shaderLC=NULL;
	shaderRGB=NULL;
    shaderBlack=NULL;
    _textureCache = NULL;
    _overlay=true;
    _fullscreen = false;
    _channelsToDraw = Mask_RGBA;
    frameData = NULL;
    _colorSpace = Lut::getLut(Lut::VIEWER);
    _mustFreeFrameData = false;
}

ViewerGL::ViewerGL(ViewerTab* viewerTab,QGLContext* context,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(context,parent,shareWidget),_viewerTab(viewerTab),_textRenderer(this),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),_lut(1),_shaderLoaded(false),_drawing(false)

{
    
    initConstructor();
    
}
ViewerGL::ViewerGL(ViewerTab* viewerTab,const QGLFormat& format,QWidget* parent ,const QGLWidget* shareWidget)
:QGLWidget(format,parent,shareWidget),_viewerTab(viewerTab),_textRenderer(this),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),_lut(1),_shaderLoaded(false),_drawing(false)

{

    initConstructor();
    
}

ViewerGL::ViewerGL(ViewerTab* viewerTab,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(parent,shareWidget),_viewerTab(viewerTab),_textRenderer(this),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),_lut(1),_shaderLoaded(false),_drawing(false)
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
	glDeleteTextures(1,&texId[0]);
    glDeleteTextures(1,&texBlack[0]);
    glDeleteBuffers(2, &texBuffer[0]);
    
    if(_mustFreeFrameData)
        free(frameData);
    
	delete _blankViewerInfos;
	delete _infoViewer;
}

void ViewerGL::updateGL(){
	//makeCurrent();
	QGLWidget::updateGL();
}

QSize ViewerGL::sizeHint() const{
    return QSize(500,500);
}
void ViewerGL::resizeGL(int width, int height){
    if(height == 0)// prevent division by 0
        height=1;
    float ap = displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
	if(transX!=0 || transY!=0){
		_ms = DRAGGING;
	}
    checkGLErrors();
    _ms = UNDEFINED;
    if(_drawing)
        ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,false,true,true);
}
void ViewerGL::paintGL()
{

    if(_drawing){
        float w = (float)width();
		float h = (float)height();
        glMatrixMode (GL_PROJECTION);
		glLoadIdentity();
        const Format& dispW = displayWindow();
        float left = -w/2.f + dispW.w()/2.f;
        float right = w/2.f + dispW.w()/2.f;
        float bottom = -h/2.f + dispW.h()/2.f;
        float top = h/2.f + dispW.h()/2.f ;
        
        glOrtho(left, right, bottom, top, -1, 1);

        glMatrixMode (GL_MODELVIEW);
		glLoadIdentity();
        
        glTranslatef(transX, -transY, 0);
        glTranslatef(_zoomCtx.zoomX, _zoomCtx.zoomY, 0);
        
        glScalef(_zoomCtx.zoomFactor, _zoomCtx.zoomFactor, 1);
        glTranslatef(-_zoomCtx.zoomX, -_zoomCtx.zoomY, 0);
        
        glEnable (GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        
        // debug (so the openGL debugger can make a breakpoint here)
              //  GLfloat d;
               // glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
        if(rgbMode())
            activateShaderRGB();
        else if(!rgbMode())
            activateShaderLC();
        glClearColor(0.0,0.0,0.0,1.0);
        glClear (GL_COLOR_BUFFER_BIT);
		glBegin (GL_POLYGON);
        glTexCoord2i (0, 0);glVertex2i (dispW.x(), _rowSpan.first);
		glTexCoord2i (0, 1);glVertex2i (dispW.x(), _rowSpan.second+1);
		glTexCoord2i (1, 1);glVertex2i (dispW.w(), _rowSpan.second+1);
		glTexCoord2i (1, 0);glVertex2i (dispW.w(), _rowSpan.first);
        
		glEnd ();
        glBindTexture(GL_TEXTURE_2D, 0);
        if(_hasHW && rgbMode() && shaderRGB){
            shaderRGB->release();
        }else if (_hasHW &&  !rgbMode() && shaderLC){
            shaderLC->release();
        }
	}else{
        if(_firstTime){
            _firstTime = false;
            fitToFormat(displayWindow());
            initBlackTex();
        }
        drawBlackTex();
    }
    if(_overlay){
        drawOverlay();
    }
}

void ViewerGL::drawOverlay(){
    checkGLErrors();
    glDisable(GL_TEXTURE_2D);
    _textRenderer.print(displayWindow().w(),0, _resolutionOverlay,QColor(233,233,233));
    
    QPoint topRight(displayWindow().w(),displayWindow().h());
    QPoint topLeft(0,displayWindow().h());
    QPoint btmLeft(0,0);
    QPoint btmRight(displayWindow().w(),0 );
    
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
        _textRenderer.print(dataWindow().x(), dataWindow().y(), _btmLeftBBOXoverlay, QColor(150,150,150));
        
        
        QPoint topRight2(dataWindow().right(), dataWindow().top());
        QPoint topLeft2(dataWindow().x(),dataWindow().top());
        QPoint btmLeft2(dataWindow().x(),dataWindow().y() );
        QPoint btmRight2(dataWindow().right(),dataWindow().y() );
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
        
        
    }
    VideoEngine* vengine = ctrlPTR->getModel()->getVideoEngine();
    if(vengine)
        vengine->drawOverlay();
    //reseting color for next pass
    glColor4f(1, 1, 1, 1);
    checkGLErrors();
}


void ViewerGL::initializeGL(){
	makeCurrent();
	initAndCheckGlExtensions();
 	glClearColor(0.0,0.0,0.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT);
    glGenTextures (1, texId);
    glGenTextures (1, texBlack);
    glGenBuffersARB(2, &texBuffer[0]);
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
    }
    
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

std::map<int,int> ViewerGL::computeRowSpan(const Box2D& displayWindow,float zoomFactor){
    std::map<int,int> ret;
    saveGLState();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    float w = (float)width();
    float h = (float)height();
    float left = -w/2.f + displayWindow.w()/2.f;
    float right = w/2.f + displayWindow.w()/2.f;
    float bottom = -h/2.f + displayWindow.h()/2.f;
    float top = h/2.f + displayWindow.h()/2.f ;
    
    glOrtho(left, right, bottom, top, -1, 1);
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glTranslatef(transX, -transY, 0);
    glTranslatef(_zoomCtx.zoomX, _zoomCtx.zoomY, 0);
    glScalef(zoomFactor, zoomFactor, 1);
    glTranslatef(-_zoomCtx.zoomX, -_zoomCtx.zoomY, 0);
    
    GLint viewport[4];
	GLfloat modelview[16];
    GLfloat projection[16];
    glGetFloatv( GL_MODELVIEW_MATRIX, modelview );
    glGetFloatv( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    float mat[16];
    float inv[16];
    _glMultMats44(mat, projection, modelview);
    if (_glInvertMatrix(mat, inv)==0) {
        cout << "failed inverting projection x modelview matrix" << endl;
    }
    float p[4];
    p[0] = (0-(float)viewport[0])/(float)viewport[2]*2.0-1.0;
    p[2] = 1.f;
    p[3] = 1.f;
    
    /*First off,we test the 1st and last row to check wether the
     image is contained in the viewer*/
    // testing top of the image
    int y = h-1;
    p[1] = (y-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
    float res = -1;
    if(!_glMultMat44Vect_onlyYComponent(&res, inv, p)){
        cout << "failed unprojection (row-span computation)" << endl;
    }
    if (res < 0) { // all the image is above the viewer
        restoreGLState();
        return ret; // do not add any row
    }
    // testing bottom now
    y = 0;
    p[1] = (y-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
    res = -1;
    if(!_glMultMat44Vect_onlyYComponent(&res, inv, p)){
        cout << "failed unprojection (row-span computation)" << endl;
    }
    if(res >= displayWindow.top()){// all the image is below the viewer
        restoreGLState();
        return ret;
    }else{
        if(res >= displayWindow.y()){
            if(res < displayWindow.top()){
                ret[res] = 0;
            }else{
                restoreGLState();
                return ret;
            }
        }
    }
    /*for all the others row (apart the first and last) we can check*/
    for(int y = 1 ; y < h; y++){
        p[1] = (y-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
        res = -1;
        if(!_glMultMat44Vect_onlyYComponent(&res, inv, p)){
            cout << "failed unprojection (row-span computation)" << endl;
        }
        if(res >= displayWindow.y()){
            if(res < displayWindow.top()){
                ret[res] = y;
            }else{
                restoreGLState();
                return ret;
            }
        }
    }
    restoreGLState();
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
	makeCurrent();
    if(!_hasHW) return;
	if(!shaderLC->bind()){
		cout << qPrintable(shaderLC->log()) << endl;
	}
	shaderLC->setUniformValue("Tex", 0);
	shaderLC->setUniformValue("yw",1.0,1.0,1.0);
    shaderLC->setUniformValue("expMult",  exposure);
    shaderLC->setUniformValue("lut", _lut);
    shaderLC->setUniformValue("byteMode", byteMode());
    
}
void ViewerGL::activateShaderRGB(){
	makeCurrent();
    if(!_hasHW) return;
	if(!shaderRGB->bind()){
		cout << qPrintable(shaderRGB->log()) << endl;
	}
    
    shaderRGB->setUniformValue("Tex", 0);
    shaderRGB->setUniformValue("byteMode", byteMode());
	shaderRGB->setUniformValue("expMult",  exposure);
    shaderRGB->setUniformValue("lut", _lut);
    
}

void ViewerGL::initShaderGLSL(){
    if(!_shaderLoaded && _hasHW){
        makeCurrent();
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

void ViewerGL::initTextures(int w,int h,GLuint texID){
    initTextureBGRA(w,h,texID);
    /*Returns immediately if shaders have already been
     initialized*/
	initShaderGLSL();
}

void ViewerGL::initTextureBGRA(int w,int h,GLuint texID){
    
    if(_textureSize.first==w && _textureSize.second==h && texID==currentTexture) return;
    
    _textureSize = make_pair(w, h);
    
	makeCurrent();
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, texID);
    
    // if the texture is zoomed, do not produce antialiasing so the user can
    // zoom to the pixel
   // if(_zoomCtx.zoomFactor >= 0.5){
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    }else{
//        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    }
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    
    
    if(byteMode()==1 || !_hasHW){
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA8, //internalFormat
                      w, h,
                      0,			// border
                      GL_BGRA,		// format
                      GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                      0);			// pixels
    }else if(byteMode()==0 && _hasHW){
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA32F_ARB, //internalFormat
                      w, h,
                      0,			// border
                      GL_RGBA,		// format
                      GL_FLOAT,	// type
                      0);			// pixels
    }
    checkGLErrors();
}
void ViewerGL::initBlackTex(){
    makeCurrent();
    
    //_viewerTab->zoomSpinbox->setValue(_zoomCtx.zoomFactor*100);
    int w = floorf(displayWindow().w()*_zoomCtx.zoomFactor);
    int h = floorf(displayWindow().h()*_zoomCtx.zoomFactor);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glBindTexture (GL_TEXTURE_2D, texBlack[0]);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D (GL_TEXTURE_2D,
                  0,			// level
                  GL_RGBA8, //internalFormat
                  w, h,
                  0,			// border
                  GL_BGRA,		// format
                  GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                  0);			// pixels
	
    
	frameData = (char*)malloc(sizeof(U32)*w*h);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, texBuffer[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, w*h*sizeof(U32), NULL, GL_DYNAMIC_DRAW_ARB);
    void* gpuBuffer = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    for( int i =0 ; i < h ; i++){
        convertRowToFitTextureBGRA(NULL, NULL, NULL,  w,i,NULL);
    }
	memcpy(gpuBuffer,frameData,w*h*sizeof(U32));
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	
    glTexSubImage2D (GL_TEXTURE_2D,
                     0,				// level
                     0,0 ,				// xoffset, yoffset
                     w, h,
                     GL_BGRA,			// format
                     GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                     0);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	free(frameData);
	frameData = 0;
    
}

void ViewerGL::drawBlackTex(){
    makeCurrent();
    
    
    float w = (float)width();
    float h = (float)height();
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    const Format& dispW = displayWindow();

    float left = -w/2.f + dispW.w()/2.f;
    float right = w/2.f + dispW.w()/2.f;
    float bottom = -h/2.f + dispW.h()/2.f;
    float top = h/2.f + dispW.h()/2.f ;
    
    
    glOrtho(left, right, bottom, top, -1, 1);
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glTranslatef(transX, -transY, 0);
    
    
    glTranslatef(_zoomCtx.zoomX, _zoomCtx.zoomY, 0);
    glScalef(_zoomCtx.zoomFactor, _zoomCtx.zoomFactor, 1);
    glTranslatef(-_zoomCtx.zoomX, -_zoomCtx.zoomY, 0);
    
    glClear (GL_COLOR_BUFFER_BIT);
    glEnable (GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, texBlack[0]);
    
    if(_hasHW && !shaderBlack->bind()){
        cout << qPrintable(shaderBlack->log()) << endl;
    }
    if(_hasHW)
        shaderBlack->setUniformValue("Tex", 0);
    glBegin (GL_POLYGON);
    glTexCoord2i (0, 0);glVertex2i (dispW.x(), dispW.y());
    glTexCoord2i (0, 1);glVertex2i (dispW.x(), dispW.h());
    glTexCoord2i (1, 1);glVertex2i (dispW.w(),dispW.h());
    glTexCoord2i (1, 0);glVertex2i (dispW.w(),dispW.y());
    glEnd ();
    if(_hasHW)
        shaderBlack->release();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ViewerGL::drawRow(Row* row){
    float zoomFactor;
    _zoomCtx.zoomFactor <= 1 ? zoomFactor = _zoomCtx.zoomFactor : zoomFactor = 1.f;
    int w = floorf(displayWindow().w() * zoomFactor);
    if(byteMode()==0 && _hasHW){
        convertRowToFitTextureBGRA_fp((*row)[Channel_red], (*row)[Channel_green], (*row)[Channel_blue],
                                      w,row->zoomedY(),(*row)[Channel_alpha]);
    }
    else{
        convertRowToFitTextureBGRA((*row)[Channel_red], (*row)[Channel_green], (*row)[Channel_blue],
                                   w,row->zoomedY(),(*row)[Channel_alpha]);
   
    }
}

bool ViewerGL::determineFrameDataContainer(std::string filename,int w,int h,ViewerGL::CACHING_MODE mode){
    if(_mustFreeFrameData){
        free(frameData);
        _mustFreeFrameData = false;
    }
    
    if(mode == TEXTURE_CACHE){ // texture caching
        TextureCache::TextureKey key(exposure,_lut,_zoomCtx.zoomFactor,
                                     w,h,byteMode(),filename,
                                     ctrlPTR->getModel()->getVideoEngine()->getCurrentTreeVersion(),
                                     _rowSpan.first,_rowSpan.second);
        TextureCache::TextureIterator found = _textureCache->isCached(key);
        U32 ret = 0;
        if(found != _textureCache->end()){
            setCurrentTexture((GLuint)found->second);
            /*flaging that it will not be needed to copy data from the current PBO
             to the texture as it already contains the results.*/
            _noDataTransfer = true;
            return true;
        }else{
            ret = _textureCache->append(key);
            size_t dataSize = 0;
            byteMode() == 1 ? dataSize = sizeof(U32)*w*h : dataSize = sizeof(float)*w*h*4;
            frameData = (char*)malloc(dataSize);
            _mustFreeFrameData = true;
            initTextures(w,h,(GLuint)ret);
            setCurrentTexture((GLuint)ret);
            return false;
        }
    }else{ // viewer caching
        // init mmaped file
        U64 treeVersion = ctrlPTR->getModel()->getVideoEngine()->getCurrentTreeVersion();
        U64 key = FrameEntry::computeHashKey(filename, treeVersion, _zoomCtx.getZoomFactor(), exposure, _lut, byteMode(), dataWindow(), displayWindow());
        
//        cout << " ADD : key computed with \n " << filename << " "
//        << treeVersion << " " <<  _zoomCtx.getZoomFactor() << " " << exposure << " " << _lut
//        << " " << byteMode() << " " << dataWindow().x() << " " << dataWindow().y() << " "  << dataWindow().right() << " "
//        << dataWindow().top() << " " << displayWindow().x() << " " << displayWindow().y() << " " <<
//        displayWindow().right() << " " << displayWindow().top() << endl;
//        cout << " KEY: " << key << endl;
        
        FrameEntry* entry = ViewerCache::getViewerCache()->addFrame(key, filename, treeVersion, _zoomCtx.getZoomFactor(), exposure, _lut, byteMode(), w, h, dataWindow(),displayWindow());
        
        if(entry){
            frameData = entry->getMappedFile()->data();
        }else{ // something happen, fallback to in-memory only version : no-caching
            cout << "WARNING: caching does not seem to work properly..falling back to pure RAM version, caching disabled." << endl;
            size_t dataSize = 0;
            byteMode() == 1 ? dataSize = sizeof(U32)*w*h : dataSize = sizeof(float)*w*h*4;
            frameData = (char*)malloc(dataSize);
            _mustFreeFrameData = true;
        }
        initTextures(w,h,texId[0]);
        setCurrentTexture(texId[0]);
        return false;
    }
}

void* ViewerGL::allocateAndMapPBO(size_t dataSize,GLuint pboID){
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pboID);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
	return glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
}

void ViewerGL::fillPBO(const char *src, void *dst, size_t byteCount){
    memcpy(dst, src, byteCount);
}
void ViewerGL::copyPBOtoTexture(int w,int h){
    if(_noDataTransfer){
        _noDataTransfer = false; // reinitialize the flag
        return;
    }
    
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, currentTexture);
    GLint currentBoundPBO;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    if (currentBoundPBO == 0) {
        return;
    }
    
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    if(byteMode()==1 || !_hasHW){
        glTexSubImage2D (GL_TEXTURE_2D,
                         0,				// level
                         0, 0,				// xoffset, yoffset
                         w, h,
                         GL_BGRA,			// format
                         GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                         0);
        
        
    }else if(byteMode() ==0 && _hasHW){
        glTexSubImage2D (GL_TEXTURE_2D,
                         0,				// level
                         0, 0 ,				// xoffset, yoffset
                         w, h,
                         GL_RGBA,			// format
                         GL_FLOAT,		// type
                         0);
        
        
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    checkGLErrors();
}

void ViewerGL::convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                          int w,int yOffset,const float* alpha){
    /*Converting one row (float32) to 8bit BGRA portion of texture. We apply a dithering algorithm based on error diffusion.
     This error diffusion will produce stripes in any image that has identical scanlines.
     To prevent this, a random horizontal position is chosen to start the error diffusion at,
     and it proceeds in both directions away from this point.*/
    
    U32* output = reinterpret_cast<U32*>(frameData);
    yOffset*=w;
    output+=yOffset;
    
    if(!_drawing){
        for(int i = 0 ; i < w ; i++){
            output[i] = toBGRA(0, 0, 0, 255);
        }
    }else if(_drawing && _colorSpace->linear()){
        int start = (int)(rand()%w);
        /* go fowards from starting point to end of line: */
        float zoomFactor;
        _zoomCtx.zoomFactor <= 1 ? zoomFactor = _zoomCtx.zoomFactor : zoomFactor = 1.f;
        for(int i = start ; i < w ; i++){
            float _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            float x = (float)i*1.f/zoomFactor;
            int nearest;
            (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
            
            r!=NULL? _r=r[nearest] : _r=0.f;
            g!=NULL? _g=g[nearest] : _g=0.f;
            b!=NULL? _b=b[nearest] : _b=0.f;
            alpha!=NULL? _a=alpha[nearest] : _a=1.f;
            
            if(!rgbMode()){
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
            }
            _r*=_a;_g*=_a;_b*=_a;
            _r*=exposure;_g*=exposure;_b*=exposure;
            a_ = _a*255;
            r_ = _r*255;
            g_ = _g*255;
            b_ = _b*255;
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        /* go backwards from starting point to start of line: */
        for(int i = start-1 ; i >= 0 ; i--){
            float _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            float x = (float)i*1.f/zoomFactor;
            int nearest;
            (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
            r!=NULL? _r=r[nearest] : _r=0.f;
            g!=NULL? _g=g[nearest] : _g=0.f;
            b!=NULL? _b=b[nearest] : _b=0.f;
            alpha!=NULL? _a=alpha[nearest] : _a=1.f;
            if(!rgbMode()){
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
            }
            _r*=_a;_g*=_a;_b*=_a;
            _r*=exposure;_g*=exposure;_b*=exposure;
            a_ = _a*255;
            r_ = _r*255;
            g_ = _g*255;
            b_ = _b*255;
            output[i] = toBGRA(r_,g_,b_,a_);
        }
    }else{ // _drawing && !linear
        /*flaging that we're using the colorspace so it doesn't try to change it in the same time
         if the user requested it*/
        _usingColorSpace = true;
        int start = (int)(rand()%w);
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        /* go fowards from starting point to end of line: */
        float zoomFactor;
        _zoomCtx.zoomFactor <= 1 ? zoomFactor = _zoomCtx.zoomFactor : zoomFactor = 1.f;
        for(int i = start ; i < w ; i++){
            float _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            float x = (float)i*1.f/zoomFactor;
            int nearest;
            (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
            r!=NULL? _r=r[nearest] : _r=0.f;
            g!=NULL? _g=g[nearest] : _g=0.f;
            b!=NULL? _b=b[nearest] : _b=0.f;
            alpha!=NULL? _a=alpha[nearest] : _a=1.f;
            
            if(!rgbMode()){
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
            }
            _r*=_a;_g*=_a;_b*=_a;
            _r*=exposure;_g*=exposure;_b*=exposure;
            error_r = (error_r&0xff) + _colorSpace->toFloatFast(_r);
            error_g = (error_g&0xff) + _colorSpace->toFloatFast(_g);
            error_b = (error_b&0xff) + _colorSpace->toFloatFast(_b);
            a_ = _a*255;
            r_ = error_r >> 8;
            g_ = error_g >> 8;
            b_ = error_b >> 8;
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        /* go backwards from starting point to start of line: */
        error_r = 0x80;
        error_g = 0x80;
        error_b = 0x80;
        
        for(int i = start-1 ; i >= 0 ; i--){
            float _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            float x = (float)i*1.f/zoomFactor;
            int nearest;
            (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
            r!=NULL? _r=r[nearest] : _r=0.f;
            g!=NULL? _g=g[nearest] : _g=0.f;
            b!=NULL? _b=b[nearest] : _b=0.f;
            alpha!=NULL? _a=alpha[nearest] : _a=1.f;
            
            if(!rgbMode()){
                _r = (_r + 1.0)*_r;
                _g = _r; _b = _r;
            }
            _r*=_a;_g*=_a;_b*=_a;
            _r*=exposure;_g*=exposure;_b*=exposure;
            error_r = (error_r&0xff) + _colorSpace->toFloatFast(_r);
            error_g = (error_g&0xff) + _colorSpace->toFloatFast(_g);
            error_b = (error_b&0xff) + _colorSpace->toFloatFast(_b);
            a_ = _a*255;
            r_ = error_r >> 8;
            g_ = error_g >> 8;
            b_ = error_b >> 8;
            output[i] = toBGRA(r_,g_,b_,a_);
            
        }
        _usingColorSpace = false;
    }
    
}

// nbbytesoutput is the size in bytes of 1 channel for the row
void ViewerGL::convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                             int  w,int yOffset,const float* alpha){
    float* output = reinterpret_cast<float*>(frameData);
    // offset in the buffer : (y)*(w) where y is the zoomedY of the row and w=nbbytes/sizeof(float)*4 = nbbytes
    yOffset *=w*sizeof(float);
    output+=yOffset;
    int index = 0;
    float zoomFactor;
    _zoomCtx.zoomFactor <= 1 ? zoomFactor = _zoomCtx.zoomFactor : zoomFactor = 1.f;
    for(int i =0 ; i < w*4 ; i+=4){
        float x = (float)index*1.f/zoomFactor;
        int nearest;
        (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
        r!=NULL? output[i]=r[nearest] : output[i]=0.f;
        g!=NULL? output[i+1]=g[nearest] : output[i+1]=0.f;
        b!=NULL? output[i+2]=b[nearest] : output[i+2]=0.f;
        alpha!=NULL? output[i+3]=alpha[nearest] : output[i+3]=1.f;
        index++;
    }
}


/*actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV*/
U32 ViewerGL::toBGRA(U32 r,U32 g,U32 b,U32 a){
    U32 res = 0x0;
    res |= b;
    res |= (g << 8);
    res |= (r << 16);
    res |= (a << 24);
    return res;
    
}

void ViewerGL::keyPressEvent ( QKeyEvent * event ){
    
    if(event->key() == Qt::Key_Space){
        releaseKeyboard();
        if(_fullscreen){
            _fullscreen=false;
            ctrlPTR->getGui()->exitFullScreen();
        }else{
            _fullscreen=true;
            ctrlPTR->getGui()->setFullScreen(dynamic_cast<TabWidget*>(_viewerTab->parentWidget()));
        }
    }
    
}
void ViewerGL::mousePressEvent(QMouseEvent *event){
    old_pos = event->pos();
    if(event->button() != Qt::MiddleButton ){
        _ms = DRAGGING;
    }
    QGLWidget::mousePressEvent(event);
}
void ViewerGL::mouseReleaseEvent(QMouseEvent *event){
    _ms = UNDEFINED;
    old_pos = event->pos();
    QGLWidget::mouseReleaseEvent(event);
}
void ViewerGL::mouseMoveEvent(QMouseEvent *event){
    QPoint pos;
    pos = openGLpos_fast((float)event->x(), event->y());
    const Format& dispW = displayWindow();
    if(pos.x() >= dispW.x() &&
       pos.x() <= dispW.w() &&
       pos.y() >= dispW.y() &&
       pos.y() <= dispW.h() &&
       event->x() >= 0 && event->x() < width() &&
       event->y() >= 0 && event->y() < height()){
        if(!_infoViewer->colorAndMouseVisible()){
            _infoViewer->showColorAndMouseInfo();
        }
        QVector4D color = getColorUnderMouse(event->x(), event->y());
        _infoViewer->setColor(color);
        _infoViewer->setMousePos(pos);
        emit infoMousePosChanged();
        if(!ctrlPTR->getModel()->getVideoEngine()->isWorking())
            emit infoColorUnderMouseChanged();
    }else{
        if(_infoViewer->colorAndMouseVisible()){
            _infoViewer->hideColorAndMouseInfo();
        }
    }
    
    
    if(_ms == DRAGGING){
        if(!ctrlPTR->getModel()->getVideoEngine()->isWorking()){
            new_pos = event->pos();
            float dx = new_pos.x() - old_pos.x();
            float dy = new_pos.y() - old_pos.y();
            transX += dx;
            transY += dy;
            old_pos = new_pos;
            if(_drawing){
                ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,false,true,true);
                
            }else{
                updateGL();
            }
        }
        
    }
}
void ViewerGL::wheelEvent(QWheelEvent *event) {
    
    if(!ctrlPTR->getModel()->getVideoEngine()->isWorking()){
        QPointF p;
        float increment=0.f;
        if(_zoomCtx.zoomFactor<1.f)
            increment=0.1;
        else
            increment=0.1*_zoomCtx.zoomFactor;
        if(event->delta() >0){
            
            _zoomCtx.zoomFactor+=increment;
            
            if(_zoomCtx.old_zoomed_pt_win != event->pos()){
                p = openGLpos_fast(event->x(), event->y());
                p.setY(displayWindow().h() - p.y());
                float dx=(p.x()-_zoomCtx.old_zoomed_pt.x());
                float dy=(p.y()-_zoomCtx.old_zoomed_pt.y());
                _zoomCtx.zoomX+=dx/2.f;
                _zoomCtx.zoomY-=dy/2.f;
                _zoomCtx.restToZoomX = dx/2.f;
                _zoomCtx.restToZoomY = dy/2.f;
                _zoomCtx.old_zoomed_pt = p;
                _zoomCtx.old_zoomed_pt_win = event->pos();
            }else{
                _zoomCtx.zoomX+=_zoomCtx.restToZoomX;
                _zoomCtx.zoomY-=_zoomCtx.restToZoomY;
                _zoomCtx.restToZoomX = 0;
                _zoomCtx.restToZoomY = 0;
            }
            
        }else if(event->delta() < 0){
            _zoomCtx.zoomFactor -= increment;
            if(_zoomCtx.zoomFactor <= 0.1){
                _zoomCtx.zoomFactor = 0.1;
            }
        }
        if(_drawing){
            ctrlPTR->getModel()->clearPlaybackCache();
            ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,false,true,true);
        }else{
            updateGL();
        }
        emit zoomChanged( _zoomCtx.zoomFactor*100);
    }
    
}
void ViewerGL::zoomSlot(int v){
    if(!ctrlPTR->getModel()->getVideoEngine()->isWorking()){
        float value = v/100.f;
        if(value <0.1) value = 0.1;
        _zoomCtx.zoomFactor = value;
        if(_drawing){
            ctrlPTR->getModel()->clearPlaybackCache();
            ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,false,true,true);
        }else{
            updateGL();
        }
    }
}
void ViewerGL::zoomSlot(QString str){
    str.remove(QChar('%'));
    zoomSlot(str.toInt());
}

QPoint ViewerGL::mousePosFromOpenGL(int x, int y){
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble winX=0, winY=0, winZ=0;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    gluProject(x,y , 0.5, modelview, projection, viewport, &winX, &winY, &winZ);
    return QPoint(winX,winY);
}
/*Returns coordinates with 0,0 at top left, Powiter inverts
 y as such : y= displayWindow().h() - y  to get the coordinates
 with 0,0 at bottom left*/
QVector3D ViewerGL::openGLpos(int x,int y){
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
    glReadPixels( x, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    return QVector3D(posX,posY,posZ);
}
QPoint ViewerGL::openGLpos_fast(int x,int y){
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX=0, winY=0;
    GLdouble posX=0, posY=0, posZ=0;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    winX = (float)x;
    winY = viewport[3]- y;
    gluUnProject( winX, winY, 1, modelview, projection, viewport, &posX, &posY, &posZ);
    return QPoint(posX,posY);
}
QVector4D ViewerGL::getColorUnderMouse(int x,int y){
    if(ctrlPTR->getModel()->getVideoEngine()->isWorking()) return QVector4D(0,0,0,0);
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX=0, winY=0;
    GLdouble posX=0, posY=0, posZ=0;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    winX = (float)x;
    winY =viewport[3]- y;
    gluUnProject( winX, winY, 1, modelview, projection, viewport, &posX, &posY, &posZ);
    if(posX < displayWindow().x() || posX >= displayWindow().w() || posY < displayWindow().y() || posY >=displayWindow().h())
        return QVector4D(0,0,0,0);
    if(byteMode()==1 || !_hasHW){
        U32 pixel;
        glReadPixels( x, winY, 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 r=0,g=0,b=0,a=0;
        b |= pixel;
        g |= (pixel >> 8);
        r |= (pixel >> 16);
        a |= (pixel >> 24);
        return QVector4D((float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f);//U32toBGRA(pixel);
    }else if(byteMode()==0 && _hasHW){
        GLfloat pixel[4];
        glReadPixels( x, winY, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        return QVector4D(pixel[0],pixel[1],pixel[2],pixel[3]);
    }
    return QVector4D(0,0,0,0);
}
QVector4D ViewerGL::U32toBGRA(U32 &c){
    QVector4D out;
    int r=0,g=0,b=0,a=0;
    b |= (c >> 24);
    g |= (((c << 8) >> 8) >> 16);
    r |= (((c << 16) >> 16) >> 8);
    a |= (c << 24) >> 24;
    out.setX((float)r/255.f);
    out.setY((float)g/255.f);
    out.setZ((float)b/255.f);
    out.setW((float)a/255.f);
    return out;
}

void ViewerGL::fitToFormat(Format displayWindow){
    float h = (float)(displayWindow.h());
    float zoomFactor = (float)height()/h;
    _zoomCtx.old_zoomed_pt_win.setX((float)width()/2.f);
    _zoomCtx.old_zoomed_pt_win.setY((float)height()/2.f);
    _zoomCtx.old_zoomed_pt.setX((float)displayWindow.w()/2.f);
    _zoomCtx.old_zoomed_pt.setY((float)displayWindow.h()/2.f);
    
    setZoomFactor(zoomFactor);
    setTranslation(0, 0);
    setZoomFactor(zoomFactor-0.05);
    resetMousePos();
    _zoomCtx.setZoomXY((float)displayWindow.w()/2.f,(float)displayWindow.h()/2.f);
}



void ViewerGL::setInfoViewer(InfoViewerWidget* i ){
    
    _infoViewer = i;
    QObject::connect(this,SIGNAL(infoMousePosChanged()), _infoViewer, SLOT(updateCoordMouse()));
    QObject::connect(this,SIGNAL(infoColorUnderMouseChanged()),_infoViewer,SLOT(updateColor()));
    QObject::connect(this,SIGNAL(infoResolutionChanged()),_infoViewer,SLOT(changeResolution()));
    QObject::connect(this,SIGNAL(infoDisplayWindowChanged()),_infoViewer,SLOT(changeDisplayWindow()));
    
    
}
void ViewerGL::setCurrentViewerInfos(ViewerInfos* viewerInfos,bool onInit,bool initBoundaries){
    _currentViewerInfos = viewerInfos;
    Format* df=ctrlPTR->getModel()->findExistingFormat(displayWindow().w(), displayWindow().h());
    if(df)
        _currentViewerInfos->getDisplayWindow().name(df->name());
    updateDataWindowAndDisplayWindowInfo();
    if(!onInit){
        _viewerTab->_currentFrameBox->setMaximum(_currentViewerInfos->lastFrame());
        _viewerTab->_currentFrameBox->setMinimum(_currentViewerInfos->firstFrame());
        int curFirstFrame = _viewerTab->frameSeeker->firstFrame();
        int curLastFrame = _viewerTab->frameSeeker->lastFrame();
        if(_currentViewerInfos->firstFrame() != curFirstFrame || _currentViewerInfos->lastFrame() != curLastFrame){
            _viewerTab->frameSeeker->setFrameRange(_currentViewerInfos->firstFrame(), _currentViewerInfos->lastFrame());
            _viewerTab->frameSeeker->setBoundaries(_currentViewerInfos->firstFrame(), _currentViewerInfos->lastFrame());
        }
    }    
}

void ViewerGL::updateDataWindowAndDisplayWindowInfo(){
    emit infoResolutionChanged();
    emit infoDisplayWindowChanged();
    _resolutionOverlay.clear();
    _resolutionOverlay.append(QString::number(displayWindow().w()));
    _resolutionOverlay.append("x");
    _resolutionOverlay.append(QString::number(displayWindow().h()));
    _btmLeftBBOXoverlay.clear();
    _btmLeftBBOXoverlay.append(QString::number(dataWindow().x()));
    _btmLeftBBOXoverlay.append(",");
    _btmLeftBBOXoverlay.append(QString::number(dataWindow().y()));
    _topRightBBOXoverlay.clear();
    _topRightBBOXoverlay.append(QString::number(dataWindow().right()));
    _topRightBBOXoverlay.append(",");
    _topRightBBOXoverlay.append(QString::number(dataWindow().top()));
    
    
    
}
void ViewerGL::updateColorSpace(QString str){
    while(_usingColorSpace){}
    if (str == "Linear(None)") {
        if(_lut != 0){ // if it wasnt already this setting
            _viewerTab->frameSeeker->clearCachedFrames();
            _colorSpace = Lut::getLut(Lut::FLOAT);
        }
        _lut = 0;
    }else if(str == "sRGB"){
        if(_lut != 1){ // if it wasnt already this setting
            _viewerTab->frameSeeker->clearCachedFrames();
            _colorSpace = Lut::getLut(Lut::VIEWER);
        }
        
        _lut = 1;
    }else if(str == "Rec.709"){
        if(_lut != 2){ // if it wasnt already this setting
            _viewerTab->frameSeeker->clearCachedFrames();
            _colorSpace = Lut::getLut(Lut::MONITOR);
        }
        _lut = 2;
    }
    if (!_drawing) {
        return;
    }
    
    if(byteMode()==1 || !_hasHW)
        ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,false,true,true);
    else
        updateGL();
    
}
void ViewerGL::updateExposure(double d){
    exposure = d;
    _viewerTab->frameSeeker->clearCachedFrames();
    if((byteMode()==1 || !_hasHW) && _drawing)
        ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,false,true,true);
    else
        updateGL();
    
}


#define SWAP_ROWS_DOUBLE(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]
int ViewerGL::_glInvertMatrix(float *m, float *out){
    float wtmp[4][8];
    float m0, m1, m2, m3, s;
    float *r0, *r1, *r2, *r3;
    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
    r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
    r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
    r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
    r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
    r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
    r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
    r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
    r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
    r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
    r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
    r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
    /* choose pivot - or die */
    if (fabsf(r3[0]) > fabsf(r2[0]))
        SWAP_ROWS_FLOAT(r3, r2);
    if (fabsf(r2[0]) > fabsf(r1[0]))
        SWAP_ROWS_FLOAT(r2, r1);
    if (fabsf(r1[0]) > fabsf(r0[0]))
        SWAP_ROWS_FLOAT(r1, r0);
    if (0.0 == r0[0])
        return 0;
    /* eliminate first variable     */
    m1 = r1[0] / r0[0];
    m2 = r2[0] / r0[0];
    m3 = r3[0] / r0[0];
    s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    /* choose pivot - or die */
    if (fabsf(r3[1]) > fabsf(r2[1]))
        SWAP_ROWS_FLOAT(r3, r2);
    if (fabsf(r2[1]) > fabsf(r1[1]))
        SWAP_ROWS_FLOAT(r2, r1);
    if (0.0 == r1[1])
        return 0;
    /* eliminate second variable */
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (0.0 != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (0.0 != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (0.0 != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (0.0 != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    /* choose pivot - or die */
    if (fabsf(r3[2]) > fabsf(r2[2]))
        SWAP_ROWS_FLOAT(r3, r2);
    if (0.0 == r2[2])
        return 0;
    /* eliminate third variable */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
    r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
    /* last check */
    if (0.0 == r3[3])
        return 0;
    s = 1.0 / r3[3];             /* now back substitute row 3 */
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;
    m2 = r2[3];                  /* now back substitute row 2 */
    s = 1.0 / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
    r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
    r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
    m1 = r1[2];                  /* now back substitute row 1 */
    s = 1.0 / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
    r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
    m0 = r0[1];                  /* now back substitute row 0 */
    s = 1.0 / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
    MAT(out, 0, 0) = r0[4];
    MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
    MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
    MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
    MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
    MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
    MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
    MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
    MAT(out, 3, 3) = r3[7];
    return 1;
}
void ViewerGL::_glMultMats44(float *result, float *matrix1, float *matrix2){
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
void ViewerGL::_glMultMat44Vect(float *resultvector, const float *matrix, const float *pvector){
    resultvector[0]=matrix[0]*pvector[0]+matrix[4]*pvector[1]+matrix[8]*pvector[2]+matrix[12]*pvector[3];
    resultvector[1]=matrix[1]*pvector[0]+matrix[5]*pvector[1]+matrix[9]*pvector[2]+matrix[13]*pvector[3];
    resultvector[2]=matrix[2]*pvector[0]+matrix[6]*pvector[1]+matrix[10]*pvector[2]+matrix[14]*pvector[3];
    resultvector[3]=matrix[3]*pvector[0]+matrix[7]*pvector[1]+matrix[11]*pvector[2]+matrix[15]*pvector[3];
}
int ViewerGL::_glMultMat44Vect_onlyYComponent(float *yComponent, const float *matrix, const float *pvector){
    float y = matrix[1]*pvector[0]+matrix[5]*pvector[1]+matrix[9]*pvector[2]+matrix[13]*pvector[3];
    float w = matrix[3]*pvector[0]+matrix[7]*pvector[1]+matrix[11]*pvector[2]+matrix[15]*pvector[3];
    if(!w) return 0;
    w = 1.f / w;
    *yComponent =  y * w;
    return 1;
}

void ViewerGL::disconnectViewer(){
    ctrlPTR->getModel()->getVideoEngine()->abort(); // aborting current work
    blankInfoForViewer();
    fitToFormat(displayWindow());
    ctrlPTR->getModel()->setVideoEngineRequirements(NULL,true);
    clearViewer();
}

/*Convenience function.
 *Ydirection is the order of fill of the display texture:
 either bottom to top or top to bottom.*/
int ViewerGL::Ydirection(){return getCurrentViewerInfos()->getYdirection();}

/*rgbMode is true when we have rgba data.
 False means it is luminance chroma*/
bool ViewerGL::rgbMode(){return getCurrentViewerInfos()->rgbMode();}

/*The dataWindow of the currentFrame(BBOX)*/
const Box2D& ViewerGL::dataWindow(){return getCurrentViewerInfos()->getDataWindow();}

/*The displayWindow of the currentFrame(Resolution)*/
const Format& ViewerGL::displayWindow(){return getCurrentViewerInfos()->getDisplayWindow();}


/*overload of QT enter/leave/resize events*/
void ViewerGL::enterEvent(QEvent *event)
{   QGLWidget::enterEvent(event);
    setFocus();
    grabMouse();
    grabKeyboard();
}
void ViewerGL::leaveEvent(QEvent *event)
{
    QGLWidget::leaveEvent(event);
    setFocus();
    releaseMouse();
    releaseKeyboard();
}
void ViewerGL::resizeEvent(QResizeEvent* event){ // public to hack the protected field
   // if(isVisible()){
        QGLWidget::resizeEvent(event);
   // }
   
}
float ViewerGL::byteMode(){
    return Settings::getPowiterCurrentSettings()->_viewerSettings.byte_mode;
}

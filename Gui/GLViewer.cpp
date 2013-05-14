//
//  GLViewer.cpp
//  Powiter
//
//  Created by Alexandre on 10/28/12.
//  Copyright (c) 2012 Alexandre. All rights reserved.
//

/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */

#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QtGui/QFont>
#include <cmath>
#include <cassert>
#include <map>
#include "Gui/GLViewer.h"
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
#include "Core/diskcache.h"
#include "Superviser/powiterFn.h"
using namespace Imf;
using namespace Imath;
using namespace std;


void ViewerGL::blankInfoForViewer(bool onInit){
    blankReaderInfo->currentFrame(0);
    blankReaderInfo->lastFrame(0);
    blankReaderInfo->firstFrame(0);
    setCurrentReaderInfo(blankReaderInfo,onInit);
}
void ViewerGL::initConstructor(){
    _hasHW=true;
    blankReaderInfo = new ReaderInfo();
    blankReaderInfo->channels(Mask_RGBA);
    blankReaderInfo->Ydirection(1);
    blankReaderInfo->rgbMode(true);
    blankReaderInfo->setDisplayWindowName("2K_Super_35(full-ap)");
    blankReaderInfo->dataWindow(0, 0, 2048, 1556);
    blankReaderInfo->pixelAspect(1.0);
    blankReaderInfo->displayWindow(0, 0, 2048, 1556);
    blankReaderInfo->firstFrame(0);
    blankReaderInfo->lastFrame(0);
    blankReaderInfo->currentFrame(0);
    blankInfoForViewer(true);
    zoomX =0; zoomY=0;
	_drawing=false;
	exposure = 1;
   // _renderingBuffer = 0;
	setMouseTracking(true);
	_ms = UNDEFINED;
    fillBuiltInZooms();
    _firstTime = true;
	shaderLC=NULL;
	shaderRGB=NULL;
    shaderBlack=NULL;
    _overlay=true;
    _fullscreen = false;
    _channelsToDraw = Mask_RGBA;
    frameData = NULL;
    _makeNewFrame = true;
    _colorSpace = Lut::getLut(Lut::VIEWER);
    _colorSpace->validate();
}

ViewerGL::ViewerGL(Controler* ctrl,float byteMode,QGLContext* context,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(context,parent,shareWidget),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),zoomFactor(1),
currentBuiltInZoom(-1.f),_lut(1),_shaderLoaded(false),_drawing(false),_byteMode(byteMode),_textRenderer(this)
{
    
    this->ctrl = ctrl;
    initConstructor();
    
}
ViewerGL::ViewerGL(Controler* ctrl,float byteMode,const QGLFormat& format,QWidget* parent ,const QGLWidget* shareWidget)
:QGLWidget(format,parent,shareWidget),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),zoomFactor(1),
currentBuiltInZoom(-1.f),_lut(1),_shaderLoaded(false),_drawing(false),_byteMode(byteMode),_textRenderer(this)

{
    this->ctrl = ctrl;
    initConstructor();
    
}

ViewerGL::ViewerGL(Controler* ctrl,float byteMode,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(parent,shareWidget),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),zoomFactor(1),
currentBuiltInZoom(-1.f),_lut(1),_shaderLoaded(false),_drawing(false),_byteMode(byteMode),_textRenderer(this)
{
    this->ctrl = ctrl;
    initConstructor();
    
}
Controler* ViewerGL::getControler(){return ctrl;}

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
	
    delete _readerInfo;
	delete blankReaderInfo;
	delete _infoViewer;
}

void ViewerGL::updateGL(){
	makeCurrent();
	QGLWidget::updateGL();
}

void ViewerGL::resizeGL(int width, int height){
    if(height == 0)// prevent division by 0
        height=1;
    float ap = _readerInfo->displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
	if(transX!=0 || transY!=0){
		_ms = DRAGGING;
	}
    _ms = UNDEFINED;
	checkGLErrors();
}
void ViewerGL::paintGL()
{
    if(_drawing){
        
        float w = (float)width();///zoomFactor;
		float h = (float)height();///zoomFactor;
        
        glMatrixMode (GL_PROJECTION);
		glLoadIdentity();
        
        
        float left = -w/2.f + displayWindow().w()/2.f;
        float right = w/2.f + displayWindow().w()/2.f;
        float bottom = -h/2.f + displayWindow().h()/2.f;
        float top = h/2.f + displayWindow().h()/2.f ;
        
        
        glOrtho(left, right, bottom, top, -1, 1);
        
        glMatrixMode (GL_MODELVIEW);
		glLoadIdentity();
        
        glTranslatef(transX, -transY, 0);
        glTranslatef(zoomX, zoomY, 0);
        glScalef(zoomFactor, zoomFactor, 1);
        glTranslatef(-zoomX, -zoomY, 0);
        
        glEnable (GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId[0]);
        
        // debug
        //GLfloat d;
        //glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
        
        if(rgbMode())
            activateShaderRGB();
        else if(!rgbMode())
            activateShaderLC();
        glClearColor(0.0,0.0,0.0,1.0);
        glClear (GL_COLOR_BUFFER_BIT);
		glBegin (GL_POLYGON);
		glTexCoord2i (0, 0);glVertex2i (_readerInfo->displayWindow().x(), _readerInfo->displayWindow().y());
		glTexCoord2i (0, 1);glVertex2i (_readerInfo->displayWindow().x(), _readerInfo->displayWindow().h());
		glTexCoord2i (1, 1);glVertex2i (_readerInfo->displayWindow().w(), _readerInfo->displayWindow().h());
		glTexCoord2i (1, 0);glVertex2i (_readerInfo->displayWindow().w(), _readerInfo->displayWindow().y());
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
            initViewer();
            initBlackTex();
        }
        drawBlackTex();
    }
    if(_overlay){
        drawOverlay();
    }
    checkGLErrors();
}

void ViewerGL::drawOverlay(){
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
    vengine->drawOverlay();
    //reseting color for next pass
    glColor4f(1, 1, 1, 1);
}

void ViewerGL::paintEvent(QPaintEvent* event){
    
    updateGL();
    
}

void ViewerGL::initializeGL(){
    // freopen("/dev/null", "w", stderr); // < TEMP FIX TO DISABLE ERRORS
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

/*Little improvment to the QT version of makeCurrent to make it faster*/
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
    shaderLC->setUniformValue("byteMode", _byteMode);
    
}
void ViewerGL::activateShaderRGB(){
	makeCurrent();
    if(!_hasHW) return;
	if(!shaderRGB->bind()){
		cout << qPrintable(shaderRGB->log()) << endl;
	}
    
    shaderRGB->setUniformValue("Tex", 0);
    shaderRGB->setUniformValue("byteMode", _byteMode);
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
        
        if(fileType()==EXR){
            shaderLC = new QGLShaderProgram(context());
            if (!shaderLC->addShaderFromSourceCode(QGLShader::Vertex, vertLC)){
                cout << qPrintable(shaderLC->log()) << endl;
            }
            if(!shaderLC->addShaderFromSourceCode(QGLShader::Fragment,fragLC))
                cout << qPrintable(shaderLC->log())<< endl;
        }
        
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

void ViewerGL::initTextures(){
    
	makeCurrent();
    int w = floorf(_readerInfo->displayWindow().w()*currentBuiltInZoom);
    int h = floorf(_readerInfo->displayWindow().h()*currentBuiltInZoom);
    initTexturesRgb(w,h);
	initShaderGLSL();
    
}




void ViewerGL::initTexturesRgb(int w,int h){
    
    if(_textureSize.first==w && _textureSize.second==h) return;
    
    _textureSize = make_pair(w, h);
    
	makeCurrent();
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, texId[0]);
    
    // if the texture is zoomed, do not produce antialiasing so the user can
    // zoom to the pixel
    if(currentBuiltInZoom >= 0.5){
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }else{
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    
    
    if(_byteMode==1 || !_hasHW){
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA8, //internalFormat
                      w, h,
                      0,			// border
                      GL_BGRA,		// format
                      GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                      0);			// pixels
    }else if(_byteMode==0 && _hasHW){
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA32F_ARB, //internalFormat
                      w, h,
                      0,			// border
                      GL_RGBA,		// format
                      GL_FLOAT,	// type
                      0);			// pixels
    }
}
void ViewerGL::initBlackTex(){
    makeCurrent();
    float zf =  closestBuiltinZoom(getZoomFactor());
    setCurrentBuiltInZoom(zf);
    ctrl->getGui()->viewer_tab->zoomSpinbox->setValue(zoomFactor*100);
    int w = floorf(_readerInfo->displayWindow().w()*currentBuiltInZoom);
    int h = floorf(_readerInfo->displayWindow().h()*currentBuiltInZoom);
    
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
    float incrementNew = builtInZooms[zf].first;
    float incrementFullsize = builtInZooms[zf].second;
    setZoomIncrement(make_pair(incrementNew,incrementFullsize));
    int y = displayWindow().y();
    int rowy = displayWindow().y();
    
	frameData = (char*)malloc(sizeof(U32)*w*h);

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, texBuffer[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, w*h*sizeof(U32), NULL, GL_DYNAMIC_DRAW_ARB);
    void* gpuBuffer = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    while( y < displayWindow().top()){
        for(int k = y; k<incrementNew+y;k++){
            convertRowToFitTextureBGRA(NULL, NULL, NULL,  w,rowy,NULL);
            
            //glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
            rowy++;
        }
        y+=incrementFullsize;
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
    
    float left = -w/2.f + displayWindow().w()/2.f;
    float right = w/2.f + displayWindow().w()/2.f;
    float bottom = -h/2.f + displayWindow().h()/2.f;
    float top = h/2.f + displayWindow().h()/2.f ;
    
    
    glOrtho(left, right, bottom, top, -1, 1);
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glTranslatef(transX, transY, 0);
    
    
    glTranslatef(zoomX, zoomY, 0);
    glScalef(zoomFactor, zoomFactor, 1);
    glTranslatef(-zoomX, -zoomY, 0);
    
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
    glTexCoord2i (0, 0);glVertex2i (_readerInfo->displayWindow().x(), _readerInfo->displayWindow().y());
    glTexCoord2i (0, 1);glVertex2i (_readerInfo->displayWindow().x(), _readerInfo->displayWindow().h());
    glTexCoord2i (1, 1);glVertex2i (_readerInfo->displayWindow().w(), _readerInfo->displayWindow().h());
    glTexCoord2i (1, 0);glVertex2i (_readerInfo->displayWindow().w(), _readerInfo->displayWindow().y());
    glEnd ();
    if(_hasHW)
        shaderBlack->release();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ViewerGL::drawRow(Row* row){
    
    int w = floorf(_readerInfo->displayWindow().w() * currentBuiltInZoom);
    
    
    if(_byteMode==0 && _hasHW){
        convertRowToFitTextureBGRA_fp((*row)[Channel_red], (*row)[Channel_green], (*row)[Channel_blue],
                                      w*sizeof(float),row->zoomedY(),(*row)[Channel_alpha]);
    }
    else{
        convertRowToFitTextureBGRA((*row)[Channel_red], (*row)[Channel_green], (*row)[Channel_blue],
                                   w,row->zoomedY(),(*row)[Channel_alpha]);
    }
    
//#ifdef __POWITER_WIN32__
//    doneCurrent();  //< NEEDED FOR MULTI THREADING
//#endif
    
}

void ViewerGL::preProcess(int nbFrameHint){
    // init mmaped file if first row
    if(_makeNewFrame){
        int w = floorf(_readerInfo->displayWindow().w() * currentBuiltInZoom);
        int h = floorf(_readerInfo->displayWindow().h()*currentBuiltInZoom);
        int frameCount = _readerInfo->lastFrame() - _readerInfo->firstFrame() +1;
        pair<char*,FrameID> p = vengine->mapNewFrame(frameCount==1 ? 0 : _readerInfo->currentFrame(),vengine->currentFrameName(), w, h, nbFrameHint);
        frameData = p.first;
        frameInfo = p.second;
        _makeNewFrame = false;
        
    }
}

std::pair<void*,size_t> ViewerGL::allocatePBO(int w,int h){
    makeCurrent();
    
    size_t dataSize = 0;
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, texBuffer[0]);
    checkGLErrors();

	if(_byteMode == 1 || !_hasHW){
		dataSize =  w*h*sizeof(U32);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
	}else{
		dataSize = w*h*sizeof(float)*4;
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
	}
    checkGLErrors();
	void* gpuBuffer = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    vengine->_cache->appendFrame(frameInfo);
    _makeNewFrame = true;
    return make_pair(gpuBuffer,dataSize);
        
}

void ViewerGL::fillPBO(const char *src, void *dst, size_t byteCount){
    memcpy(dst, src, byteCount);    
}
void ViewerGL::copyPBOtoTexture(int w,int h){
    makeCurrent();
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texId[0]);
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    if(_byteMode==1 || !_hasHW){
        checkGLErrors();
        
        glTexSubImage2D (GL_TEXTURE_2D,
                         0,				// level
                         0, 0,				// xoffset, yoffset
                         w, h,
                         GL_BGRA,			// format
                         GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                         0);
        
        
    }else if(_byteMode ==0 && _hasHW){
        glTexSubImage2D (GL_TEXTURE_2D,
                         0,				// level
                         0, 0 ,				// xoffset, yoffset
                         w, h,
                         GL_RGBA,			// format
                         GL_FLOAT,		// type
                         0);
        
        
    }
	checkGLErrors();
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    
    

}

void ViewerGL::convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,size_t nbBytesOutput,int yOffset,const float* alpha){
    
    /*Converting one row (float32) to 8bit BGRA texture. We apply a dithering algorithm based on error diffusion.
     This error diffusion will produce stripes in any image that has identical scanlines.
     To prevent this, a random horizontal position is chosen to start the error diffusion at,
     and it proceeds in both directions away from this point.*/
    
    U32* output = reinterpret_cast<U32*>(frameData);
    yOffset*=nbBytesOutput;
    output+=yOffset;
    U32* end = output + nbBytesOutput;
    
    int downScaleIncrement = (int)zoomIncrement.first; // number of pixels to keep in the scan
    int fullSizeIncrement = (int)zoomIncrement.second; // number of pixels to scan per cycle
    
    int start = nbBytesOutput/fullSizeIncrement;
    int incrementCount = (int)(rand()%start);
    start = incrementCount*fullSizeIncrement;
    int itOld =  start;
    U32* itNew = output + (incrementCount*downScaleIncrement);
    unsigned error_r = 0x80;
    unsigned error_g = 0x80;
    unsigned error_b = 0x80;
    /*This boolean is here to avoid computing 2 times the starting pixel.
     The first pass we just skip the starting pixel*/
    bool skipStartingPixel = true;
    /* go fowards from starting point to end of line: */
    while(itNew < end){
        U32* kept = itNew;
        while(kept < downScaleIncrement+itNew && kept<end){
            if(!skipStartingPixel){
                float _r,_g,_b,_a;
                U32 r_,g_,b_,a_;
                if(_drawing){
                    r!=NULL? _r=r[itOld] : _r=0.f;
                    g!=NULL? _g=g[itOld] : _g=0.f;
                    b!=NULL? _b=b[itOld] : _b=0.f;
                    alpha!=NULL? _a=alpha[itOld] : _a=1.f;
                    
                    if(!rgbMode()){
                        _r = (_r + 1.0)*_r;
                        _g = _r; _b = _r;
                    }
                    _r*=_a;_g*=_a;_b*=_a;
                    _r*=exposure;_g*=exposure;_b*=exposure;
                    if(!_colorSpace->linear()){
                        error_r = (error_r&0xff) + _colorSpace->lookup_toByteLUT(_r);
                        error_g = (error_g&0xff) + _colorSpace->lookup_toByteLUT(_g);
                        error_b = (error_b&0xff) + _colorSpace->lookup_toByteLUT(_b);
                        a_ = _a*255;
                        r_ = error_r >> 8;
                        g_ = error_g >> 8;
                        b_ = error_b >> 8;
                    }else{
                        a_ = _a*255;
                        r_ = _r*255;
                        g_ = _g*255;
                        b_ = _b*255;
                    }
                    
                }else{
                    r_ = g_ = b_ = 0;
                    a_ = 255;
                }
                
                *kept = toBGRA(r_,g_,b_,a_);
            }else{
                skipStartingPixel = false;
            }
            kept++;
            itOld++;
        }
        itNew += downScaleIncrement;
        itOld += (fullSizeIncrement - downScaleIncrement);
    }
    
    /* go backwards from starting point to start of line: */
    itOld =  start;
    itNew = output + ((incrementCount)*downScaleIncrement);
    error_r = 0x80;
    error_g = 0x80;
    error_b = 0x80;
    
    while(itNew >= output){
        U32* kept = itNew;
        while(kept > itNew-downScaleIncrement && kept>=output){
            float _r,_g,_b,_a;
            U8 r_,g_,b_,a_;
            if(_drawing){
                r!=NULL? _r=r[itOld] : _r=0.f;
                g!=NULL? _g=g[itOld] : _g=0.f;
                b!=NULL? _b=b[itOld] : _b=0.f;
                alpha!=NULL? _a=alpha[itOld] : _a=1.f;
                
                if(!rgbMode()){
                    _r = (_r + 1.0)*_r;
                    _g = _r; _b = _r;
                }
                _r*=_a;_g*=_a;_b*=_a;
                _r*=exposure;_g*=exposure;_b*=exposure;
                if(!_colorSpace->linear()){
                    error_r = (error_r&0xff) + _colorSpace->lookup_toByteLUT(_r);
                    error_g = (error_g&0xff) + _colorSpace->lookup_toByteLUT(_g);
                    error_b = (error_b&0xff) + _colorSpace->lookup_toByteLUT(_b);
                    a_ = _a*255;
                    r_ = error_r >> 8;
                    g_ = error_g >> 8;
                    b_ = error_b >> 8;
                }else{
                    a_ = _a*255;
                    r_ = _r*255;
                    g_ = _g*255;
                    b_ = _b*255;
                }
                
                
            }else{
                r_ = g_ = b_ = 0;
                a_ = 255;
            }
            
            *kept = toBGRA(r_,g_,b_,a_);
            kept--;
            itOld--;
        }
        itNew-= downScaleIncrement;
        itOld -= (fullSizeIncrement-downScaleIncrement);
    }
    
}
// nbbytesoutput is the size in bytes of 1 channel for the row
void ViewerGL::convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                             size_t nbBytesOutput,int yOffset,const float* alpha){
    float* output = reinterpret_cast<float*>(frameData);
    // offset in the buffer : (y)*(w) where y is the zoomedY of the row and w=nbbytes/sizeof(float)*4 = nbbytes
    yOffset *=nbBytesOutput;
    output+=yOffset;
    
    float downScaleIncrement = zoomIncrement.first; // number of rows to keep in the scan
    float fullSizeIncrement = zoomIncrement.second; // number of rows to scan per cycle
    
    int itOld = 0;
    int itNew = 0;
    while(itNew < nbBytesOutput){
        int kept = itNew;
        while(kept < downScaleIncrement*4+itNew && kept<nbBytesOutput){
            r!=NULL? output[kept]=r[itOld] : output[kept]=0.f;
            g!=NULL? output[kept+1]=g[itOld] : output[kept+1]=0.f;
            b!=NULL? output[kept+2]=b[itOld] : output[kept+2]=0.f;
            alpha!=NULL? output[kept+3]=alpha[itOld] : output[kept+3]=1.f;
            kept+=4;
            itOld++;
        }
        
        itNew+= downScaleIncrement*4;
        itOld += (fullSizeIncrement - downScaleIncrement);
    }
    checkGLErrors();
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
            ctrl->getGui()->rightDock->show();
            ctrl->getGui()->WorkShop->show();
        }else{
            _fullscreen=true;
            ctrl->getGui()->rightDock->hide();
            ctrl->getGui()->WorkShop->hide();
            
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
    pos = openGLpos((float)event->x(), event->y());
    if(pos.x() >= _readerInfo->displayWindow().x() && pos.x() <= _readerInfo->displayWindow().w() && pos.y() >=_readerInfo->displayWindow().y() && pos.y() <= _readerInfo->displayWindow().h()){
        if(!_infoViewer->colorAndMouseVisible()){
            _infoViewer->showColorAndMouseInfo();
        }
        QVector4D color = getColorUnderMouse(event->x(), event->y());
        _infoViewer->setColor(color);
        _infoViewer->setMousePos(pos);
        emit infoMousePosChanged();
        if(!vengine->isWorking())
            emit infoColorUnderMouseChanged();
    }else{
        if(_infoViewer->colorAndMouseVisible()){
            _infoViewer->hideColorAndMouseInfo();
        }
    }
    
    
    if(_ms == DRAGGING){
        new_pos = event->pos();
        float dx = new_pos.x() - old_pos.x();
        float dy = new_pos.y() - old_pos.y();
        transX += dx;
        transY += dy;
        updateGL();
        old_pos = new_pos;
        
    }
}
void ViewerGL::wheelEvent(QWheelEvent *event) {
    QPointF p;
    float increment=0.f;
    if(zoomFactor<1.f)
        increment=0.1;
    else
        increment=0.1*zoomFactor;
	if(!vengine->isWorking()){
		if(event->delta() >0){
            
            zoomFactor+=increment;
            
            if(old_zoomed_pt_win != event->pos()){
                p = openGLpos(event->x(), event->y());
                p.setY(displayWindow().h() - p.y());
                float dx=(p.x()-old_zoomed_pt.x());
                float dy=(p.y()-old_zoomed_pt.y());
                zoomX+=dx/2.f;
                zoomY-=dy/2.f;
                restToZoomX = dx/2.f;
                restToZoomY = dy/2.f;
                old_zoomed_pt = p;
                old_zoomed_pt_win = event->pos();
            }else{
                zoomX+=restToZoomX;
                zoomY-=restToZoomY;
                restToZoomX = 0;
                restToZoomY = 0;
            }
            zoomIn();
            
            
            
		}else if(event->delta() < 0){
			zoomFactor -= increment;
            zoomOut();
        }
        
        emit zoomChanged(zoomFactor*100);
	}
    
    
}
void ViewerGL::zoomSlot(int v){
    float value = v/100.f;
    if(!vengine->isWorking()){
        if(v<zoomFactor){ // zoom out
            zoomFactor = value;
            zoomOut();
        }else{
            zoomFactor = value;
            zoomIn();
        }
    }
}

QPoint ViewerGL::mousePosFromOpenGL(int x, int y){
    GLint viewport[4];
	GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble winX=0, winY=0, winZ=0;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    double ap = _readerInfo->displayWindow().pixel_aspect();
    if(ap < 1){
        viewport[0]=0;viewport[1]=0;viewport[2]=width();viewport[3]=height()*ap;
    }
    gluProject(x,y , 0.5, modelview, projection, viewport, &winX, &winY, &winZ);
    return QPoint(winX,winY);
}
/*Returns coordinates with 0,0 at top left, Powiter inverts
 y as such : y= displayWindow().h() - y  to get the coordinates
 with 0,0 at bottom left*/
QPoint ViewerGL::openGLpos(int x,int y){
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
    return QPoint(posX,posY);
}
QVector4D ViewerGL::getColorUnderMouse(int x,int y){
    if(vengine->isWorking()) return QVector4D(0,0,0,0);
    
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX=0, winY=0, winZ=0;
    GLdouble posX=0, posY=0, posZ=0;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    winX = (float)x;
    winY =viewport[3]- y;
    glReadPixels( x, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    if(posX < displayWindow().x() || posX >= displayWindow().w() || posY < displayWindow().y() || posY >=displayWindow().h())
        return QVector4D(0,0,0,0);
    if(_byteMode==1 || !_hasHW){
        U32 pixel;
        glReadPixels( x, winY, 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 r=0,g=0,b=0,a=0;
        b |= pixel;
        g |= (pixel >> 8);
        r |= (pixel >> 16);
        a |= (pixel >> 24);
        return QVector4D((float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f);//U32toBGRA(pixel);
    }else if(_byteMode==0 && _hasHW){
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

void ViewerGL::fillBuiltInZooms(){
    builtInZooms[1.f/10.f] = make_pair(1,10);
    builtInZooms[1.f/4.f] = make_pair(1,4);
    builtInZooms[1.f/2.f] = make_pair(1,2);
    builtInZooms[3.f/4.f] = make_pair(3,4);
    builtInZooms[9.f/10.f]= make_pair(9,10);
    builtInZooms[1.f] = make_pair(1,1);
    
}
float ViewerGL::closestBuiltinZoom(float v){
    std::map<float, pair<int,int> >::iterator it = builtInZooms.begin();
    std::map<float, pair<int,int> >::iterator suiv = it;
    ++suiv;
    if( v < it->first)
        return it->first;
    for(;it!=builtInZooms.end();it++){
        if(it->first == v)
            return it->first;
        else if(suiv!=builtInZooms.end() && it->first <v && suiv->first >= v)
            return suiv->first;
        else if(suiv==builtInZooms.end() && it->first < v  )
            return 1.f;
        suiv++;
    }
    return -1.f;
}


float ViewerGL::inferiorBuiltinZoom(float v){
    map<float, pair<int,int> >::iterator it=builtInZooms.begin(); it++;
    map<float, pair<int,int> >::iterator prec =builtInZooms.begin();
    for(;it!=builtInZooms.end();it++){
        float value = it->first;
        if( value == v &&  value != builtInZooms.begin()->first)
            return prec->first;
        else if(value==v && value==builtInZooms.begin()->first)
            return value;
        prec++;
    }
    return -1.f;
}
float ViewerGL::superiorBuiltinZoom(float v){
    map<float, pair<int,int> >::iterator suiv=builtInZooms.begin(); suiv++;
    map<float, pair<int,int> >::iterator it =builtInZooms.begin();
    for(;it!=builtInZooms.end();it++){
        float value = it->first;
        if( value == v &&  value != 1.f)
            return suiv->first;
        else if(value==v && value==1.f)
            return 1.f;
        suiv++;
    }
    return -1.f;
}
void ViewerGL::initViewer(){
    float h = (float)(displayWindow().h());
    float zoomFactor = (float)height()/h;
    old_zoomed_pt_win.setX((float)width()/2.f);
    old_zoomed_pt_win.setY((float)height()/2.f);
    old_zoomed_pt.setX((float)displayWindow().w()/2.f);
    old_zoomed_pt.setY((float)displayWindow().h()/2.f);
    
    setZoomFactor(zoomFactor);
    setTranslation(0, 0);
    setZoomFactor(zoomFactor-0.05);
    resetMousePos();
    setZoomXY((float)displayWindow().w()/2.f,(float)displayWindow().h()/2.f);
}

void ViewerGL::zoomIn(){
    if(zoomFactor<=1.f){
        if(zoomFactor > currentBuiltInZoom && _drawing){
            currentBuiltInZoom = superiorBuiltinZoom(currentBuiltInZoom);
            int w = floorf(_readerInfo->displayWindow().w()*currentBuiltInZoom);
            int h = floorf(_readerInfo->displayWindow().h()*currentBuiltInZoom);
            initTexturesRgb(w,h);
            vengine->_cache->clearPlayBackCache();
            vengine->videoEngine(1,false,true,true);
        }
        
    }else
        updateGL();
    
}
void ViewerGL::zoomOut(){
    if(zoomFactor <= 0.1){
        zoomFactor = 0.1;
    }
    if(zoomFactor<=1.f){
        float inf = inferiorBuiltinZoom(currentBuiltInZoom);
        if(zoomFactor < inf && _drawing){
            currentBuiltInZoom = inf;
            int w = floorf(_readerInfo->displayWindow().w()*currentBuiltInZoom);
            int h = floorf(_readerInfo->displayWindow().h()*currentBuiltInZoom);
            initTexturesRgb(w,h);
            vengine->_cache->clearPlayBackCache();
            vengine->videoEngine(1,false,true,true);
        }
    }else
        updateGL();
}

void ViewerGL::setInfoViewer(InfoViewerWidget* i ){
    
    _infoViewer = i;
    QObject::connect(this, SIGNAL(infoMousePosChanged()), _infoViewer, SLOT(updateCoordMouse()));
    QObject::connect(this,SIGNAL(infoColorUnderMouseChanged()),_infoViewer,SLOT(updateColor()));
    QObject::connect(this,SIGNAL(infoResolutionChanged()),_infoViewer,SLOT(changeResolution()));
    QObject::connect(this,SIGNAL(infoDisplayWindowChanged()),_infoViewer,SLOT(changeDisplayWindow()));
    
    
}
void ViewerGL::setCurrentReaderInfo(ReaderInfo* info,bool onInit,bool initBoundaries){
    
    _readerInfo = info;
    DisplayFormat* df=ctrl->getModel()->findExistingFormat(_readerInfo->displayWindow().w(), _readerInfo->displayWindow().h());
    if(df)
        _readerInfo->setDisplayWindowName(df->name());
    updateDataWindowAndDisplayWindowInfo();
    if(!onInit){
        ctrl->getGui()->viewer_tab->frameNumberBox->setMaximum(_readerInfo->lastFrame());
        ctrl->getGui()->viewer_tab->frameNumberBox->setMinimum(_readerInfo->firstFrame());
    }
    if(!onInit)
        ctrl->getGui()->viewer_tab->frameSeeker->setFrameRange(_readerInfo->firstFrame(), _readerInfo->lastFrame(),initBoundaries);
    if((_readerInfo->lastFrame() - _readerInfo->firstFrame())==0){
        emit frameChanged(0);
        
    }else{
        emit frameChanged(_readerInfo->currentFrame());
    }
}

void ViewerGL::updateDataWindowAndDisplayWindowInfo(){
    emit infoResolutionChanged();
    emit infoDisplayWindowChanged();
    _resolutionOverlay.clear();
    _resolutionOverlay.append(QString::number(_readerInfo->displayWindow().w()));
    _resolutionOverlay.append("x");
    _resolutionOverlay.append(QString::number(_readerInfo->displayWindow().h()));
    _btmLeftBBOXoverlay.clear();
    _btmLeftBBOXoverlay.append(QString::number(_readerInfo->dataWindow().x()));
    _btmLeftBBOXoverlay.append(",");
    _btmLeftBBOXoverlay.append(QString::number(_readerInfo->dataWindow().y()));
    _topRightBBOXoverlay.clear();
    _topRightBBOXoverlay.append(QString::number(_readerInfo->dataWindow().right()));
    _topRightBBOXoverlay.append(",");
    _topRightBBOXoverlay.append(QString::number(_readerInfo->dataWindow().top()));
    
    
    
}
void ViewerGL::updateColorSpace(QString str){
    if (str == "Linear(None)") {
        if(_lut != 0){ // if it wasnt already this setting
            delete _colorSpace;
            _colorSpace = Lut::getLut(Lut::FLOAT);
            _colorSpace->validate();
        }
        _lut = 0;
    }else if(str == "sRGB"){
        if(_lut != 1){ // if it wasnt already this setting
            delete _colorSpace;
            _colorSpace = Lut::getLut(Lut::VIEWER);
            _colorSpace->validate();
        }
        
        _lut = 1;
    }else if(str == "Rec.709"){
        if(_lut != 2){ // if it wasnt already this setting
            delete _colorSpace;
            _colorSpace = Lut::getLut(Lut::MONITOR);
            _colorSpace->validate();
        }
        
        _lut = 2;
    }
    
    
    if(_byteMode==1 || !_hasHW)
        vengine->computeFrameRequest(true,true,false);
    updateGL();
    
}
void ViewerGL::updateExposure(double d){
    exposure = d;
    
    if(_byteMode==1 || !_hasHW)
        vengine->computeFrameRequest(true,true,false);
    updateGL();
    
}


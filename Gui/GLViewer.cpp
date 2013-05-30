//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */

#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QtGui/QFont>
#include <QtWidgets/QDockWidget>
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
#include "Core/viewercache.h"
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
    blankReaderInfo->currentFrame(0);
    blankReaderInfo->lastFrame(0);
    blankReaderInfo->firstFrame(0);
    setCurrentReaderInfo(blankReaderInfo,onInit);
}
void ViewerGL::initConstructor(){
    _hasHW=true;
    _readerInfo = new ReaderInfo;
    blankReaderInfo = new ReaderInfo;
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
	_drawing=false;
	exposure = 1;
	setMouseTracking(true);
	_ms = UNDEFINED;
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
    _mustFreeFrameData = false;
}

ViewerGL::ViewerGL(Controler* ctrl,float byteMode,QGLContext* context,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(context,parent,shareWidget),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),_lut(1),_shaderLoaded(false),_drawing(false),_byteMode(byteMode),_textRenderer(this)
{
    
    this->ctrl = ctrl;
    initConstructor();
    
}
ViewerGL::ViewerGL(Controler* ctrl,float byteMode,const QGLFormat& format,QWidget* parent ,const QGLWidget* shareWidget)
:QGLWidget(format,parent,shareWidget),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),_lut(1),_shaderLoaded(false),_drawing(false),_byteMode(byteMode),_textRenderer(this)

{
    this->ctrl = ctrl;
    initConstructor();
    
}

ViewerGL::ViewerGL(Controler* ctrl,float byteMode,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(parent,shareWidget),Ysampling(1),_roi(0,0,0,0),transX(0),transY(0),_lut(1),_shaderLoaded(false),_drawing(false),_byteMode(byteMode),_textRenderer(this)
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
    delete _colorSpace;
}

void ViewerGL::updateGL(){
	//makeCurrent();
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
    if(_drawing)
        vengine->videoEngine(1,false,true,true);
	checkGLErrors();
}
void ViewerGL::paintGL()
{
    if(_drawing){
        
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
        
        glTranslatef(transX, -transY, 0);
        glTranslatef(_zoomCtx.zoomX, _zoomCtx.zoomY, 0);
        
        /*scale is here to adjust the zoomFactor because the texture size is not really
         at the size expected for this zoomFactor (since we removed some rows).
         In the case the frame fit entirely in the viewer, scale should be equal
         to 1.*/
		//float scale = (float)_textureSize.second/(float)displayWindow().h(); //(float)_textureSize.second/zoomedHeight();
        
        glScalef(_zoomCtx.zoomFactor, _zoomCtx.zoomFactor, 1);
        glTranslatef(-_zoomCtx.zoomX, -_zoomCtx.zoomY, 0);
        
        glEnable (GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId[0]);
        
        // debug (so the openGL debugger can make a breakpoint here)
        //                GLfloat d;
        //                glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
        
        if(rgbMode())
            activateShaderRGB();
        else if(!rgbMode())
            activateShaderLC();
        glClearColor(0.0,0.0,0.0,1.0);
        glClear (GL_COLOR_BUFFER_BIT);
		glBegin (GL_POLYGON);
        //		glTexCoord2i (0, 0);glVertex2i (_readerInfo->displayWindow().x(), _readerInfo->displayWindow().y());
        //		glTexCoord2i (0, 1);glVertex2i (_readerInfo->displayWindow().x(), _readerInfo->displayWindow().h());
        //		glTexCoord2i (1, 1);glVertex2i (_readerInfo->displayWindow().w(), _readerInfo->displayWindow().h());
        //		glTexCoord2i (1, 0);glVertex2i (_readerInfo->displayWindow().w(), _readerInfo->displayWindow().y());
        glTexCoord2i (0, 0);glVertex2i (_readerInfo->displayWindow().x(), _rowSpan.first);
		glTexCoord2i (0, 1);glVertex2i (_readerInfo->displayWindow().x(), _rowSpan.second+1);
		glTexCoord2i (1, 1);glVertex2i (_readerInfo->displayWindow().w(), _rowSpan.second+1);
		glTexCoord2i (1, 0);glVertex2i (_readerInfo->displayWindow().w(), _rowSpan.first);
        
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

std::map<int,int> ViewerGL::computeRowSpan(Format displayWindow,float zoomFactor){
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
    if(res >= displayWindow.h()){// all the image is below the viewer
        restoreGLState();
        return ret;
    }else{
        if(res >= displayWindow.y()){
            if(res < displayWindow.h()){
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
            if(res < displayWindow.h()){
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

void ViewerGL::initTextures(int w,int h){
    
	makeCurrent();
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
    if(_zoomCtx.zoomFactor >= 0.5){
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
    
    ctrl->getGui()->viewer_tab->zoomSpinbox->setValue(_zoomCtx.zoomFactor*100);
    int w = floorf(_readerInfo->displayWindow().w()*_zoomCtx.zoomFactor);
    int h = floorf(_readerInfo->displayWindow().h()*_zoomCtx.zoomFactor);
    
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
    
    float left = -w/2.f + displayWindow().w()/2.f;
    float right = w/2.f + displayWindow().w()/2.f;
    float bottom = -h/2.f + displayWindow().h()/2.f;
    float top = h/2.f + displayWindow().h()/2.f ;
    
    
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
    
    int w = floorf(_readerInfo->displayWindow().w() * _zoomCtx.zoomFactor);
    if(_byteMode==0 && _hasHW){
        convertRowToFitTextureBGRA_fp((*row)[Channel_red], (*row)[Channel_green], (*row)[Channel_blue],
                                      w,row->zoomedY(),(*row)[Channel_alpha]);
    }
    else{
        convertRowToFitTextureBGRA((*row)[Channel_red], (*row)[Channel_green], (*row)[Channel_blue],
                                   w,row->zoomedY(),(*row)[Channel_alpha]);
    }
}

void ViewerGL::preProcess(std::string filename,int nbFrameHint,int w,int h){
    // init mmaped file
    if(_makeNewFrame){
        int frameCount = _readerInfo->lastFrame() - _readerInfo->firstFrame() +1;
        if(_mustFreeFrameData){
            free(frameData);
            _mustFreeFrameData = false;
        }
        
        QPoint top = mousePosFromOpenGL(0, displayWindow().h()-1);
        QPoint btm = mousePosFromOpenGL(0, displayWindow().y());
        if(top.y() >= height() || btm.y() < 0){
            size_t dataSize = 0;
            _byteMode == 1 ? dataSize = sizeof(U32)*w*h : dataSize = sizeof(float)*w*h*4;
            frameData = (char*)malloc(dataSize);
            _mustFreeFrameData = true;
        }else{
            pair<char*,ViewerCache::FrameID> p = vengine->mapNewFrame(frameCount == 1 ? 0 : _readerInfo->currentFrame(),
                                                                      filename,
                                                                      w, h,
                                                                      nbFrameHint);
            frameData = p.first;
            frameInfo = p.second;
            _makeNewFrame = false;
        }
        
        
        
    }
}

std::pair<void*,size_t> ViewerGL::allocatePBO(int w,int h){
    //makeCurrent();
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
    if(!_makeNewFrame){
        vengine->_viewerCache->appendFrame(frameInfo);
        _makeNewFrame = true;
    }
    return make_pair(gpuBuffer,dataSize);
    
}

void ViewerGL::fillPBO(const char *src, void *dst, size_t byteCount){
    memcpy(dst, src, byteCount);
}
void ViewerGL::copyPBOtoTexture(int w,int h){
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texId[0]);
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    if(_byteMode==1 || !_hasHW){
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
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    checkGLErrors();
}
void ViewerGL::convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                          int w,int yOffset,const float* alpha){
    /*Converting one row (float32) to 8bit BGRA texture. We apply a dithering algorithm based on error diffusion.
     This error diffusion will produce stripes in any image that has identical scanlines.
     To prevent this, a random horizontal position is chosen to start the error diffusion at,
     and it proceeds in both directions away from this point.*/
    
    /*flaging that we're using the colorspace so it doesn't try to change it in the same time
     if the user requested it*/
    _usingColorSpace = true;
    
    U32* output = reinterpret_cast<U32*>(frameData);
    yOffset*=w;
    output+=yOffset;
    int start = (int)(rand()%w);
    unsigned error_r = 0x80;
    unsigned error_g = 0x80;
    unsigned error_b = 0x80;
    /* go fowards from starting point to end of line: */
    for(int i = start ; i < w ; i++){
        float _r,_g,_b,_a;
        U8 r_,g_,b_,a_;
        float x = (float)i*1.f/_zoomCtx.zoomFactor;
        int nearest;
        (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
        if(_drawing){
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
            if(!_colorSpace->linear()){
                error_r = (error_r&0xff) + _colorSpace->toFloatFast(_r);
                error_g = (error_g&0xff) + _colorSpace->toFloatFast(_g);
                error_b = (error_b&0xff) + _colorSpace->toFloatFast(_b);
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
        output[i] = toBGRA(r_,g_,b_,a_);
    }
    /* go backwards from starting point to start of line: */
    error_r = 0x80;
    error_g = 0x80;
    error_b = 0x80;
    
    for(int i = start-1 ; i >= 0 ; i--){
        float _r,_g,_b,_a;
        U8 r_,g_,b_,a_;
        float x = (float)i*1.f/_zoomCtx.zoomFactor;
        int nearest;
        (x-floor(x) < ceil(x) - x) ? nearest = floor(x) : nearest = ceil(x);
        if(_drawing){
            
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
            if(!_colorSpace->linear()){
                error_r = (error_r&0xff) + _colorSpace->toFloatFast(_r);
                error_g = (error_g&0xff) + _colorSpace->toFloatFast(_g);
                error_b = (error_b&0xff) + _colorSpace->toFloatFast(_b);
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
        output[i] = toBGRA(r_,g_,b_,a_);
    }
    _usingColorSpace = false;
}

// nbbytesoutput is the size in bytes of 1 channel for the row
void ViewerGL::convertRowToFitTextureBGRA_fp(const float* r,const float* g,const float* b,
                                             int  w,int yOffset,const float* alpha){
    float* output = reinterpret_cast<float*>(frameData);
    // offset in the buffer : (y)*(w) where y is the zoomedY of the row and w=nbbytes/sizeof(float)*4 = nbbytes
    yOffset *=w*sizeof(float);
    output+=yOffset;
    int index = 0;
    for(int i =0 ; i < w*4 ; i+=4){
        float x = (float)index*1.f/_zoomCtx.zoomFactor;
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
    pos = openGLpos_fast((float)event->x(), event->y());
    if(pos.x() >= _readerInfo->displayWindow().x() &&
       pos.x() <= _readerInfo->displayWindow().w() &&
       pos.y() >=_readerInfo->displayWindow().y() &&
       pos.y() <= _readerInfo->displayWindow().h() &&
       event->x() >= 0 && event->x() < width() &&
       event->y() >= 0 && event->y() < height()){
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
        if(!vengine->isWorking()){
            new_pos = event->pos();
            float dx = new_pos.x() - old_pos.x();
            float dy = new_pos.y() - old_pos.y();
            transX += dx;
            transY += dy;
            old_pos = new_pos;
            if(_drawing){
                vengine->videoEngine(1,false,true,true);
                
            }else{
                updateGL();
            }
        }
        
    }
}
    void ViewerGL::wheelEvent(QWheelEvent *event) {
        
        if(!vengine->isWorking()){
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
                vengine->_viewerCache->clearPlayBackCache();
                vengine->videoEngine(1,false,true,true);
            }else{
                updateGL();
            }
            emit zoomChanged( _zoomCtx.zoomFactor*100);
        }
        
    }
    void ViewerGL::zoomSlot(int v){
        if(!vengine->isWorking()){
            float value = v/100.f;
            if(value <0.1) value = 0.1;
            _zoomCtx.zoomFactor = value;
            if(_drawing){
                vengine->_viewerCache->clearPlayBackCache();
                vengine->videoEngine(1,false,true,true);
            }
            updateGL();
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
        QObject::connect(this, SIGNAL(infoMousePosChanged()), _infoViewer, SLOT(updateCoordMouse()));
        QObject::connect(this,SIGNAL(infoColorUnderMouseChanged()),_infoViewer,SLOT(updateColor()));
        QObject::connect(this,SIGNAL(infoResolutionChanged()),_infoViewer,SLOT(changeResolution()));
        QObject::connect(this,SIGNAL(infoDisplayWindowChanged()),_infoViewer,SLOT(changeDisplayWindow()));
        
        
    }
    void ViewerGL::setCurrentReaderInfo(ReaderInfo* info,bool onInit,bool initBoundaries){
        _readerInfo->copy(info);
        Format* df=ctrl->getModel()->findExistingFormat(_readerInfo->displayWindow().w(), _readerInfo->displayWindow().h());
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
        while(_usingColorSpace){}
        if (str == "Linear(None)") {
            if(_lut != 0){ // if it wasnt already this setting
                delete _colorSpace;
                ctrl->getGui()->viewer_tab->frameSeeker->clearCachedFrames();
                _colorSpace = Lut::getLut(Lut::FLOAT);
                _colorSpace->validate();
            }
            _lut = 0;
        }else if(str == "sRGB"){
            if(_lut != 1){ // if it wasnt already this setting
                delete _colorSpace;
                ctrl->getGui()->viewer_tab->frameSeeker->clearCachedFrames();
                _colorSpace = Lut::getLut(Lut::VIEWER);
                _colorSpace->validate();
            }
            
            _lut = 1;
        }else if(str == "Rec.709"){
            if(_lut != 2){ // if it wasnt already this setting
                delete _colorSpace;
                ctrl->getGui()->viewer_tab->frameSeeker->clearCachedFrames();
                _colorSpace = Lut::getLut(Lut::MONITOR);
                _colorSpace->validate();
            }
            
            _lut = 2;
        }
        if(_byteMode==1 || !_hasHW)
            vengine->videoEngine(1,false,true,true);
        updateGL();
        
    }
    void ViewerGL::updateExposure(double d){
        exposure = d;
        ctrl->getGui()->viewer_tab->frameSeeker->clearCachedFrames();
        if(_byteMode==1 || !_hasHW)
            vengine->videoEngine(1,false,true,true);
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
    

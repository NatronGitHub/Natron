//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */








/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */
#include "Gui/GLViewer.h"
#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QDockWidget>
#include <QtOpenGL/QGLShaderProgram>
#include <cassert>
#include <map>
#include "Gui/tabwidget.h"
#include "Core/VideoEngine.h"
#include "Gui/shaders.h"
#include "Core/lookUpTables.h"
#include "Superviser/controler.h"
#include "Gui/InfoViewerWidget.h"
#include "Core/model.h"
#include "Gui/FeedbackSpinBox.h"
#include "Core/lutclasses.h"
#include "Gui/timeline.h"
#include "Core/viewercache.h"
#include "Core/settings.h"
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include "Core/mappedfile.h"
#include "Superviser/powiterFn.h"

using namespace Imf;
using namespace Imath;
using namespace std;

const double pi= 3.14159265358979323846264338327950288419717;

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
    _blankViewerInfos->setChannels(Powiter::Mask_RGBA);
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
	shaderLC=NULL;
	shaderRGB=NULL;
    shaderBlack=NULL;
    _textureCache = NULL;
    _overlay=true;
    frameData = NULL;
    _colorSpace = Lut::getLut(Lut::VIEWER);
    _currentDisplayTexture = 0;
    _pBOmapped = false;
}

ViewerGL::ViewerGL(QGLContext* context,QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(context,parent,shareWidget),
_textRenderer(this),
_shaderLoaded(false),
_lut(1),
_drawing(false)
{
    
    initConstructor();
    
}
ViewerGL::ViewerGL(const QGLFormat& format,QWidget* parent ,const QGLWidget* shareWidget)
:QGLWidget(format,parent,shareWidget),
_textRenderer(this),
_shaderLoaded(false),
_lut(1),
_drawing(false)
{
    
    initConstructor();
    
}

ViewerGL::ViewerGL(QWidget* parent,const QGLWidget* shareWidget)
:QGLWidget(parent,shareWidget),
_textRenderer(this),
_shaderLoaded(false),
_lut(1),
_drawing(false)
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
    delete _viewerCacheTexture;
    glDeleteTextures(1,&_blackTexId[0]);
    glDeleteBuffers(2, &_pboIds[0]);
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
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
    checkGLErrors();
    _ms = UNDEFINED;
    if(_drawing)
        emit engineNeeded();
}
void ViewerGL::paintGL()
{
    
    float w = (float)width();
    float h = (float)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    const Format& dispW = displayWindow();
    float bottom = _zoomCtx._bottom;
    float left = _zoomCtx._left;
    float top =  bottom +  h / _zoomCtx._zoomFactor;
    float right = left +  w / _zoomCtx._zoomFactor;
    glOrtho(left, right, bottom, top, -1, 1);
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    
    glEnable (GL_TEXTURE_2D);
    if(_drawing){
        glBindTexture(GL_TEXTURE_2D, _currentDisplayTexture->getTexID());
        // debug (so the OpenGL debugger can make a breakpoint here)
        // GLfloat d;
        //glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
        if(rgbMode())
            activateShaderRGB();
        else if(!rgbMode())
            activateShaderLC();
    }else{
        glBindTexture(GL_TEXTURE_2D, _blackTexId[0]);
        if(_hasHW && !shaderBlack->bind()){
            cout << qPrintable(shaderBlack->log()) << endl;
        }
        if(_hasHW)
            shaderBlack->setUniformValue("Tex", 0);
        _rowSpan.first = 0;
        _rowSpan.second = dispW.h()-1;
    }
    glClearColor(0.0,0.0,0.0,1.0);
    glClear (GL_COLOR_BUFFER_BIT);
    glBegin (GL_POLYGON);
    glTexCoord2i (0, 0);glVertex2i (dispW.x(), _rowSpan.first);
    glTexCoord2i (0, 1);glVertex2i (dispW.x(), _rowSpan.second+1);
    glTexCoord2i (1, 1);glVertex2i (dispW.w(), _rowSpan.second+1);
    glTexCoord2i (1, 0);glVertex2i (dispW.w(), _rowSpan.first);
    glEnd ();
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
    _viewerCacheTexture = new TextureEntry;
    glGenTextures (1, _blackTexId);
    glGenBuffersARB(2, &_pboIds[0]);
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
    
    
    initBlackTex();
    
    
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

void ViewerGL::computeRowSpan(std::map<int,int>& ret,const Box2D& displayWindow){
    /*First off,we test the 1st and last row to check wether the
     image is contained in the viewer*/
    // testing top of the image
    int y = 0;
    float res = -1;
    res = toImgCoordinates_fast(0,y).y();
    if (res < 0) { // all the image is above the viewer
        return ; // do not add any row
    }
    // testing bottom now
    y = height()-1;
    res = toImgCoordinates_fast(0,y).y();
    /*for all the others row (apart the first and last) we can check.
     */
    while(res < 0 && y >= 0){
        /*while y is an invalid line, iterate*/
        --y;
        res = toImgCoordinates_fast(0,y).y();
    }
    while(res < displayWindow.h() && y >= 0){
        /*y is a valid line in widget coord && res contains the image y coord.*/
        ret[res] = y;
        --y;
        res = toImgCoordinates_fast(0,y).y();
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
    fitToFormat(displayWindow());
    int w = floorf(displayWindow().w()*_zoomCtx._zoomFactor);
    int h = floorf(displayWindow().h()*_zoomCtx._zoomFactor);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glBindTexture (GL_TEXTURE_2D, _blackTexId[0]);
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
	
    
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, w*h*sizeof(U32), NULL, GL_DYNAMIC_DRAW_ARB);
    frameData = (char*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    U32* output = reinterpret_cast<U32*>(frameData);
    for(int i = 0 ; i < w*h ; i++){
        output[i] = toBGRA(0, 0, 0, 255);
    }
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	
    glTexSubImage2D (GL_TEXTURE_2D,
                     0,				// level
                     0,0 ,				// xoffset, yoffset
                     w, h,
                     GL_BGRA,			// format
                     GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                     0);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    
}



void ViewerGL::drawRow(const float* r,const float* g,const float* b,const float* a,float zoomFactor ,int zoomedY){
    if (zoomFactor > 1.f) {
        zoomFactor = 1.f;
    } 
    int w = floorf(displayWindow().w() * zoomFactor);
    if(byteMode()==0 && _hasHW){
        convertRowToFitTextureBGRA_fp(r,g,b,w,zoomFactor,zoomedY,a);
    }
    else{
        convertRowToFitTextureBGRA(r,g,b,w,zoomFactor,zoomedY,a);
        
    }
}

size_t ViewerGL::determineFrameDataContainer(U64 key,int w,int h){
    
    size_t dataSize = 0;
    TextureEntry::DataType type;
    if(byteMode() == 1 || !_hasHW){
        dataSize = sizeof(U32)*w*h;
        type = TextureEntry::BYTE;
    }else{
        dataSize = sizeof(float)*w*h*4;
        type = TextureEntry::FLOAT;
    }
    //TextureEntry* ret = _textureCache->generateTexture(key,w,h,type);
    //setCurrentDisplayTexture(ret);
    /*MUST map the PBO AFTER that we allocate the texture.*/
    frameData = (char*)allocateAndMapPBO(dataSize,_pboIds[0]);
    assert(frameData);
    checkGLErrors();
    return dataSize;
}

void* ViewerGL::allocateAndMapPBO(size_t dataSize,GLuint pboID){
    _pBOmapped = true;
    //cout << "    + mapping PBO" << endl;
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pboID);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
	return glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
}

void ViewerGL::fillPBO(const char *src, void *dst, size_t byteCount){
    memcpy(dst, src, byteCount);
}
void ViewerGL::copyPBOToNewTexture(TextureEntry* texture,int width,int height){
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    TextureEntry::DataType type;
    if(byteMode() == 1 || !_hasHW){
        type = TextureEntry::BYTE;
    }else{
        type = TextureEntry::FLOAT;
    }
    texture->allocate(width,height,type);
    _textureCache->addTexture(texture);
    setCurrentDisplayTexture(texture);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    frameData = 0;
    _pBOmapped = false;

}

void ViewerGL::copyPBOToExistingTexture(){
    
    
    int w = _currentDisplayTexture->w();
    int h = _currentDisplayTexture->h();
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _currentDisplayTexture->getTexID());
    GLint currentBoundPBO;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    if (currentBoundPBO == 0) {
        cout << "(ViewerGL::copyPBOtoTexture WARNING: Attempting to copy data from a PBO that is not mapped." << endl;
        return;
    }
    
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    if(_currentDisplayTexture->type() == TextureEntry::BYTE){
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,				// level
                        0, 0,				// xoffset, yoffset
                        w, h,
                        GL_BGRA,			// format
                        GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                        0);
        
        
    }else if(_currentDisplayTexture->type() == TextureEntry::FLOAT){
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,				// level
                        0, 0 ,				// xoffset, yoffset
                        w, h,
                        GL_RGBA,			// format
                        GL_FLOAT,		// type
                        0);
        
        
    }
    // cout << "    - unmapping PBO" << endl;
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    frameData = 0;
    _pBOmapped = false;
    checkGLErrors();
}

void ViewerGL::convertRowToFitTextureBGRA(const float* r,const float* g,const float* b,
                                          int w,float zoomFactor,int yOffset,const float* alpha){
    /*Converting one row (float32) to 8bit BGRA portion of texture. We apply a dithering algorithm based on error diffusion.
     This error diffusion will produce stripes in any image that has identical scanlines.
     To prevent this, a random horizontal position is chosen to start the error diffusion at,
     and it proceeds in both directions away from this point.*/
    assert(frameData);
    U32* output = reinterpret_cast<U32*>(frameData);
    yOffset*=w;
    output+=yOffset;
    
    if(_colorSpace->linear()){
        int start = (int)(rand()%w);
        /* go fowards from starting point to end of line: */
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
    }else{ // !linear
        /*flaging that we're using the colorspace so it doesn't try to change it in the same time
         if the user requested it*/
        _usingColorSpace = true;
        int start = (int)(rand()%w);
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        /* go fowards from starting point to end of line: */
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
                                             int  w,float zoomFactor,int yOffset,const float* alpha){
    assert(frameData);
    float* output = reinterpret_cast<float*>(frameData);
    // offset in the buffer : (y)*(w) where y is the zoomedY of the row and w=nbbytes/sizeof(float)*4 = nbbytes
    yOffset *=w*sizeof(float);
    output+=yOffset;
    int index = 0;
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
        _ms = DRAGGING;
    }
    QGLWidget::mousePressEvent(event);
}
void ViewerGL::mouseReleaseEvent(QMouseEvent *event){
    _ms = UNDEFINED;
    QGLWidget::mouseReleaseEvent(event);
}
void ViewerGL::mouseMoveEvent(QMouseEvent *event){
    QPointF pos;
    pos = toImgCoordinates_fast((float)event->x(), event->y());
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
        _infoViewer->setMousePos(QPoint(pos.x(),pos.y()));
        emit infoMousePosChanged();
        if(!ctrlPTR->getModel()->getVideoEngine()->isWorking())
            emit infoColorUnderMouseChanged();
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
            _zoomCtx._bottom += (oldClick_opengl.y() - newClick_opengl.y());
            _zoomCtx._left += (oldClick_opengl.x() - newClick_opengl.x());
            _zoomCtx._oldClick = newClick;
            if(_drawing)
                emit engineNeeded();
            else
                updateGL();
        //  }
        
    }
}
void ViewerGL::wheelEvent(QWheelEvent *event) {
    // if(!ctrlPTR->getModel()->getVideoEngine()->isWorking() || !_drawing){
        
        float newZoomFactor;
        if(event->delta() >0){
            newZoomFactor =   _zoomCtx._zoomFactor*pow(1.01f,event->delta());
        }else {
            newZoomFactor = _zoomCtx._zoomFactor/pow(1.01f,-event->delta());
            if(newZoomFactor <= 0.1){
                newZoomFactor = 0.1;
            }
        }
        QPointF zoomCenter = toImgCoordinates_fast(event->x(), event->y());
        float zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
        _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio;
        _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
        
        _zoomCtx._zoomFactor = newZoomFactor;
        if(_drawing){
            ctrlPTR->getModel()->clearPlaybackCache();
            emit engineNeeded();
        }
       else
           updateGL();

    // }
    
    emit zoomChanged( _zoomCtx._zoomFactor*100);
    
    
}
void ViewerGL::zoomSlot(int v){
    if(!ctrlPTR->getModel()->getVideoEngine()->isWorking()){
        float value = v/100.f;
        if(value < 0.1f) value = 0.1f;
        _zoomCtx._zoomFactor = value;
        if(_drawing){
            ctrlPTR->getModel()->clearPlaybackCache();
            emit engineNeeded();
        }else{
            updateGL();
        }
    }
}
void ViewerGL::zoomSlot(QString str){
    str.remove(QChar('%'));
    zoomSlot(str.toInt());
}

QPoint ViewerGL::toWidgetCoordinates(int x, int y){
    float w = (float)width() ;
    float h = (float)height();
    float bottom = _zoomCtx._bottom;
    float left = _zoomCtx._left;
    float top =  bottom +  h / _zoomCtx._zoomFactor;
    float right = left +  w / _zoomCtx._zoomFactor;
    return QPoint((((float)x - left)/(right - left))*w,(((float)y - top)/(bottom - top))*h);
}
/*Returns coordinates with 0,0 at top left, Powiter inverts
 y as such : y= displayWindow().h() - y  to get the coordinates
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
    glReadPixels( x, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    return QVector3D(posX,posY,posZ);
}

QVector4D ViewerGL::getColorUnderMouse(int x,int y){
    if(ctrlPTR->getModel()->getVideoEngine()->isWorking()) return QVector4D(0,0,0,0);
    QPointF pos = toImgCoordinates_fast(x, y);
    if(pos.x() < displayWindow().x() || pos.x() >= displayWindow().w() || pos.y() < displayWindow().y() || pos.y() >=displayWindow().h())
        return QVector4D(0,0,0,0);
    if(byteMode()==1 || !_hasHW){
        U32 pixel;
        glReadPixels( x, height()-y, 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 r=0,g=0,b=0,a=0;
        b |= pixel;
        g |= (pixel >> 8);
        r |= (pixel >> 16);
        a |= (pixel >> 24);
        return QVector4D((float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f);//U32toBGRA(pixel);
    }else if(byteMode()==0 && _hasHW){
        GLfloat pixel[4];
        glReadPixels( x, height()-y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        return QVector4D(pixel[0],pixel[1],pixel[2],pixel[3]);
    }
    return QVector4D(0,0,0,0);
}

void ViewerGL::fitToFormat(Format displayWindow){
    float h = (float)(displayWindow.h());
    float w = (float)(displayWindow.w());
    float zoomFactor = (float)height()/h;
    setZoomFactor(zoomFactor-0.05);
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
    Format* df=ctrlPTR->getModel()->findExistingFormat(displayWindow().w(), displayWindow().h());
    if(df)
        _currentViewerInfos->getDisplayWindow().name(df->name());
    updateDataWindowAndDisplayWindowInfo();
}

void ViewerGL::updateDataWindowAndDisplayWindowInfo(){
    emit infoResolutionChanged();
    emit infoDataWindowChanged();
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
            _colorSpace = Lut::getLut(Lut::FLOAT);
        }
        _lut = 0;
    }else if(str == "sRGB"){
        if(_lut != 1){ // if it wasnt already this setting
            _colorSpace = Lut::getLut(Lut::VIEWER);
        }
        
        _lut = 1;
    }else if(str == "Rec.709"){
        if(_lut != 2){ // if it wasnt already this setting
            _colorSpace = Lut::getLut(Lut::MONITOR);
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


//#define SWAP_ROWS_DOUBLE(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
//#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
//#define MAT(m,r,c) (m)[(c)*4+(r)]
//int ViewerGL::_glInvertMatrix(float *m, float *out){
//    float wtmp[4][8];
//    float m0, m1, m2, m3, s;
//    float *r0, *r1, *r2, *r3;
//    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
//    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
//    r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
//    r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
//    r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
//    r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
//    r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
//    r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
//    r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
//    r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
//    r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
//    r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
//    r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
//    /* choose pivot - or die */
//    if (fabsf(r3[0]) > fabsf(r2[0]))
//        SWAP_ROWS_FLOAT(r3, r2);
//    if (fabsf(r2[0]) > fabsf(r1[0]))
//        SWAP_ROWS_FLOAT(r2, r1);
//    if (fabsf(r1[0]) > fabsf(r0[0]))
//        SWAP_ROWS_FLOAT(r1, r0);
//    if (0.0 == r0[0])
//        return 0;
//    /* eliminate first variable     */
//    m1 = r1[0] / r0[0];
//    m2 = r2[0] / r0[0];
//    m3 = r3[0] / r0[0];
//    s = r0[1];
//    r1[1] -= m1 * s;
//    r2[1] -= m2 * s;
//    r3[1] -= m3 * s;
//    s = r0[2];
//    r1[2] -= m1 * s;
//    r2[2] -= m2 * s;
//    r3[2] -= m3 * s;
//    s = r0[3];
//    r1[3] -= m1 * s;
//    r2[3] -= m2 * s;
//    r3[3] -= m3 * s;
//    s = r0[4];
//    if (s != 0.0) {
//        r1[4] -= m1 * s;
//        r2[4] -= m2 * s;
//        r3[4] -= m3 * s;
//    }
//    s = r0[5];
//    if (s != 0.0) {
//        r1[5] -= m1 * s;
//        r2[5] -= m2 * s;
//        r3[5] -= m3 * s;
//    }
//    s = r0[6];
//    if (s != 0.0) {
//        r1[6] -= m1 * s;
//        r2[6] -= m2 * s;
//        r3[6] -= m3 * s;
//    }
//    s = r0[7];
//    if (s != 0.0) {
//        r1[7] -= m1 * s;
//        r2[7] -= m2 * s;
//        r3[7] -= m3 * s;
//    }
//    /* choose pivot - or die */
//    if (fabsf(r3[1]) > fabsf(r2[1]))
//        SWAP_ROWS_FLOAT(r3, r2);
//    if (fabsf(r2[1]) > fabsf(r1[1]))
//        SWAP_ROWS_FLOAT(r2, r1);
//    if (0.0 == r1[1])
//        return 0;
//    /* eliminate second variable */
//    m2 = r2[1] / r1[1];
//    m3 = r3[1] / r1[1];
//    r2[2] -= m2 * r1[2];
//    r3[2] -= m3 * r1[2];
//    r2[3] -= m2 * r1[3];
//    r3[3] -= m3 * r1[3];
//    s = r1[4];
//    if (0.0 != s) {
//        r2[4] -= m2 * s;
//        r3[4] -= m3 * s;
//    }
//    s = r1[5];
//    if (0.0 != s) {
//        r2[5] -= m2 * s;
//        r3[5] -= m3 * s;
//    }
//    s = r1[6];
//    if (0.0 != s) {
//        r2[6] -= m2 * s;
//        r3[6] -= m3 * s;
//    }
//    s = r1[7];
//    if (0.0 != s) {
//        r2[7] -= m2 * s;
//        r3[7] -= m3 * s;
//    }
//    /* choose pivot - or die */
//    if (fabsf(r3[2]) > fabsf(r2[2]))
//        SWAP_ROWS_FLOAT(r3, r2);
//    if (0.0 == r2[2])
//        return 0;
//    /* eliminate third variable */
//    m3 = r3[2] / r2[2];
//    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
//    r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
//    /* last check */
//    if (0.0 == r3[3])
//        return 0;
//    s = 1.0 / r3[3];             /* now back substitute row 3 */
//    r3[4] *= s;
//    r3[5] *= s;
//    r3[6] *= s;
//    r3[7] *= s;
//    m2 = r2[3];                  /* now back substitute row 2 */
//    s = 1.0 / r2[2];
//    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
//    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
//    m1 = r1[3];
//    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
//    r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
//    m0 = r0[3];
//    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
//    r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
//    m1 = r1[2];                  /* now back substitute row 1 */
//    s = 1.0 / r1[1];
//    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
//    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
//    m0 = r0[2];
//    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
//    r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
//    m0 = r0[1];                  /* now back substitute row 0 */
//    s = 1.0 / r0[0];
//    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
//    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
//    MAT(out, 0, 0) = r0[4];
//    MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
//    MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
//    MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
//    MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
//    MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
//    MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
//    MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
//    MAT(out, 3, 3) = r3[7];
//    return 1;
//}

bool ViewerGL::_glInvertMatrix(float *m ,float* invOut){
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
    
    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;
    
    return true;
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
int ViewerGL::_glMultMat44Vect_onlyYComponent(float *yComponent,const M44f& matrix, const Eigen::Vector4f& pvector){
    Eigen::Vector4f ret = matrix * pvector;
    if(!ret.w()) return 0;
    ret.w() = 1.f / ret.w();
    *yComponent =  ret.y() * ret.w();
    return 1;
}

/*Replicate of the glOrtho func, but for a custom matrix*/
void ViewerGL::_glOrthoFromIdentity(M44f& matrix,float left,float right,float bottom,float top,float near_,float far_){
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
void ViewerGL::_glScale(M44f& result,const M44f& matrix,float x,float y,float z){
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
    //   _glMultMats44(result, matrix, scale);
}

/*Replicate of the glTranslate function but for a custom matrix*/
void ViewerGL::_glTranslate(M44f& result,const M44f& matrix,float x,float y,float z){
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
    //_glMultMats44(result, matrix, translate);
}

/*Replicate of the glRotate function but for a custom matrix*/
void ViewerGL::_glRotate(M44f& result,const M44f& matrix,float a,float x,float y,float z){
    
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
    //    _glMultMats44(result, matrix, rotate);
    
}

/*Replicate of the glLoadIdentity function but for a custom matrix*/
void ViewerGL::_glLoadIdentity(M44f& matrix){
    
    matrix.setIdentity();
    //    matrix[0] = 1.0f;
    //    matrix[1] = 0.0f;
    //    matrix[2] = 0.0f;
    //    matrix[3] = 0.0f;
    //
    //    matrix[4] = 0.0f;
    //    matrix[5] = 1.0f;
    //    matrix[6] = 0.0f;
    //    matrix[7] = 0.0f;
    //
    //    matrix[8] = 0.0f;
    //    matrix[9] = 0.0f;
    //    matrix[10] = 1.0f;
    //    matrix[11] = 0.0f;
    //
    //    matrix[12] = 0.0f;
    //    matrix[13] = 0.0f;
    //    matrix[14] = 0.0f;
    //    matrix[15] = 1.0f;
}

void ViewerGL::disconnectViewer(){
    ctrlPTR->getModel()->getVideoEngine()->abort(); // aborting current work
    blankInfoForViewer();
    fitToFormat(displayWindow());
    ctrlPTR->getModel()->setVideoEngineRequirements(NULL,true);
    clearViewer();
}

/*rgbMode is true when we have rgba data.
 False means it is luminance chroma*/
bool ViewerGL::rgbMode(){return getCurrentViewerInfos()->rgbMode();}

/*The dataWindow of the currentFrame(BBOX)*/
const Box2D& ViewerGL::dataWindow(){return getCurrentViewerInfos()->getDataWindow();}

/*The displayWindow of the currentFrame(Resolution)*/
const Format& ViewerGL::displayWindow(){return getCurrentViewerInfos()->getDisplayWindow();}

/*display black in the viewer*/
void ViewerGL::clearViewer(){
    drawing(false);
    updateGL();
}

/*overload of QT enter/leave/resize events*/
void ViewerGL::enterEvent(QEvent *event)
{   QGLWidget::enterEvent(event);
    setFocus();
    // grabMouse();
    grabKeyboard();
}
void ViewerGL::leaveEvent(QEvent *event)
{
    QGLWidget::leaveEvent(event);
    setFocus();
    // releaseMouse();
    releaseKeyboard();
}
void ViewerGL::resizeEvent(QResizeEvent* event){ // public to hack the protected field
                                                 // if(isVisible()){
    QGLWidget::resizeEvent(event);
    // }
    
}

void ViewerGL::setCurrentDisplayTexture(TextureEntry* texture){
    if(_currentDisplayTexture)
        _currentDisplayTexture->returnToNormalPriority();
    _currentDisplayTexture = texture;
    texture->preventFromDeletion();
}
float ViewerGL::byteMode() const {
    return Settings::getPowiterCurrentSettings()->_viewerSettings.byte_mode;
}

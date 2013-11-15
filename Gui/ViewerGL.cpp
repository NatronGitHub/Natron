//  Natron
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

#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QMenu>
#include <QDockWidget>
#include <QtOpenGL/QGLShaderProgram>
#include <QtCore/QEvent>
#include <QtGui/QPainter>
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QKeyEvent>
#include <QtCore/QFile>

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
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"
#include "Engine/MemoryFile.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Timer.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"


/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */

//using namespace Imf;
//using namespace Imath;
using namespace Natron;
using std::cout; using std::endl;

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
    const TextureRect& r = _displayingImage ? _defaultDisplayTexture->getTextureRect() : _blackTex->getTextureRect();
    const RectI& img = _clipToDisplayWindow ? getDisplayWindow() : getRoD();
    GLfloat vertices[32] = {
        (GLfloat)img.left() ,(GLfloat)img.top()  , //0
        (GLfloat)r.x       , (GLfloat)img.top()  , //1
        (GLfloat)r.r + 1.f , (GLfloat)img.top()  , //2
        (GLfloat)img.right(),(GLfloat)img.top()  , //3
        (GLfloat)img.left(), (GLfloat)r.t + 1.f, //4
        (GLfloat)r.x      ,  (GLfloat)r.t + 1.f, //5
        (GLfloat)r.r + 1.f,  (GLfloat)r.t + 1.f, //6
        (GLfloat)img.right(),(GLfloat)r.t + 1.f, //7
        (GLfloat)img.left() ,(GLfloat)r.y      , //8
        (GLfloat)r.x      ,  (GLfloat)r.y      , //9
        (GLfloat)r.r + 1.f,  (GLfloat)r.y      , //10
        (GLfloat)img.right(),(GLfloat)r.y      , //11
        (GLfloat)img.left(), (GLfloat)img.bottom(), //12
        (GLfloat)r.x      ,  (GLfloat)img.bottom(), //13
        (GLfloat)r.r + 1.f,  (GLfloat)img.bottom(), //14
        (GLfloat)img.right(),(GLfloat)img.bottom() //15
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

#if 0
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
#endif

namespace
{
    const int TEXTURE_SIZE = 256;
    
    struct CharBitmap
    {
        GLuint _texID;
        uint _w;
        uint _h;
        GLfloat _xTexCoords[2];
        GLfloat _yTexCoords[2];
    };
    
}

namespace Natron{
    
    struct TextRendererPrivate
    {
        TextRendererPrivate(const QFont& font);
        
        ~TextRendererPrivate();
        
        void newTransparantTexture();
        
        CharBitmap* createCharacter(QChar c, const QColor &color);
        
        void clearCache();
        
        QFont _font;
        
        QFontMetrics _fontMetrics;
        
        QHash<ushort, std::vector<std::pair<QColor,CharBitmap> > > _bitmapsCache;
        
        std::list<GLuint> _usedTextures;
        
        GLint _xOffset;
        
        GLint _yOffset;
    };
    
    TextRendererPrivate::TextRendererPrivate(const QFont& font)
    : _font(font)
    , _fontMetrics(font)
    , _xOffset(0)
    , _yOffset(0)
    {
    }
    
    TextRendererPrivate::~TextRendererPrivate()
    {
        clearCache();
    }
    
    void TextRendererPrivate::clearCache(){
        foreach (GLuint texture, _usedTextures){
            glDeleteTextures(1, &texture);
        }
    }
    void TextRendererPrivate::newTransparantTexture()
    {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        
        QImage image(TEXTURE_SIZE, TEXTURE_SIZE, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        image = QGLWidget::convertToGLFormat(image);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_SIZE, TEXTURE_SIZE,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
        
        _usedTextures.push_back(texture);
    }
    
    CharBitmap* TextRendererPrivate::createCharacter(QChar c,const QColor& color)
    {
        ushort unic = c.unicode();
        
        //c is already in the cache
        QHash<ushort, std::vector<std::pair<QColor,CharBitmap> > >::iterator it = _bitmapsCache.find(unic);
        std::vector<std::pair<QColor,CharBitmap> >::iterator it2;
        if (it != _bitmapsCache.end()){
            for(it2 = it.value().begin();it2 != it.value().end();++it2){
                const QColor& found = (*it2).first;
                if(found.redF() == color.redF() &&
                   found.greenF() == color.greenF() &&
                   found.blueF() == color.blueF() &&
                   found.alphaF() == color.alphaF()){
                    break;
                }
            }
            if(it2 != it.value().end()){
                return &(*it2).second;
            }
        }
        
        GLint currentBoundPBO;
        //if a pbo is already mapped, return, this would make the glTex** calls fail
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
        if (currentBoundPBO != 0) {
            return NULL;
        }
        
        if (_usedTextures.empty())
            newTransparantTexture();
        
        GLuint texture = _usedTextures.back();
        
        GLsizei width = _fontMetrics.width(c);
        GLsizei height = _fontMetrics.height();
        
        //render into a new transparant pixmap using QPainter
        QImage image(width, height,QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter;
        painter.begin(&image);
        painter.setRenderHints(QPainter::HighQualityAntialiasing
                               | QPainter::TextAntialiasing);
        painter.setFont(_font);
        painter.setPen(color);
        
        painter.drawText(0, _fontMetrics.ascent(), c);
        painter.end();
        
        
        //fill the texture with the QImage
        image = QGLWidget::convertToGLFormat(image);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, _xOffset, _yOffset, width, height, GL_RGBA,
                        GL_UNSIGNED_BYTE, image.bits());
        
        
        if(it == _bitmapsCache.end()){
            std::vector<std::pair<QColor,CharBitmap> > newHash;
            it = _bitmapsCache.insert(unic,newHash);
        }
        CharBitmap character;
        character._texID = texture;
        
        character._w = width;
        character._h = height;
        
        character._xTexCoords[0] = (GLfloat)_xOffset / TEXTURE_SIZE;
        character._xTexCoords[1] = (GLfloat)(_xOffset + width) / TEXTURE_SIZE;
        
        character._yTexCoords[0] = (GLfloat)_yOffset / TEXTURE_SIZE;
        character._yTexCoords[1] = (GLfloat)(_yOffset + height) / TEXTURE_SIZE;
        
        it.value().push_back(std::make_pair(color,character)); // insert a new charactr
        
        _xOffset += width;
        if (_xOffset + _fontMetrics.maxWidth() >= TEXTURE_SIZE)
        {
            _xOffset = 1;
            _yOffset += height;
        }
        if (_yOffset + _fontMetrics.height() >= TEXTURE_SIZE)
        {
            newTransparantTexture();
            _yOffset = 1;
        }
        return &(it.value().back().second);
    }
    
    
    TextRenderer::TextRenderer() :
    _renderers()
    {
    }
    
    TextRenderer::~TextRenderer()
    {
        for(FontRenderers::iterator it = _renderers.begin();it!= _renderers.end();++it){
            delete (*it).second;
        }
    }
    
    
    void TextRenderer::renderText(float x, float y, const QString &text,const QColor& color,const QFont& font)
    {
        
        TextRendererPrivate* _imp = NULL;
        FontRenderers::iterator it;
        for(it = _renderers.begin() ;it!=_renderers.end();++it){
            if((*it).first == font){
                break;
            }
        }
        if(it != _renderers.end()){
            _imp  = (*it).second;
        }else{
            _imp = new TextRendererPrivate(font);
            _renderers.push_back(std::make_pair(font,_imp));
        }
        glColor4f(1., 1., 1., 1.);
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GLuint texture = 0;
        glTranslatef(x, y, 0);
        for (int i = 0; i < text.length(); ++i)
        {
            CharBitmap *c = _imp->createCharacter(text[i],color);
            if(!c)
                continue;
            if (texture != c->_texID)
            {
                texture = c->_texID;
                glBindTexture(GL_TEXTURE_2D, texture);
            }
            
            glBegin(GL_QUADS);
            glTexCoord2f(c->_xTexCoords[0], c->_yTexCoords[0]);glVertex2f(0, 0);
            glTexCoord2f(c->_xTexCoords[1], c->_yTexCoords[0]);glVertex2f(c->_w, 0);
            glTexCoord2f(c->_xTexCoords[1], c->_yTexCoords[1]);glVertex2f(c->_w, c->_h);
            glTexCoord2f(c->_xTexCoords[0], c->_yTexCoords[1]);glVertex2f(0, c->_h);
            glEnd();
            
            glTranslatef(c->_w, 0, 0);
        }
        glPopMatrix();
        glPopAttrib();
        checkGLErrors();
        glColor4f(1., 1., 1., 1.);

        
    }
    
} //namespace Natron


void ViewerGL::initConstructor(){
    
    _hasHW = true;
    _blankViewerInfos.setChannels(Natron::Mask_RGBA);
    Format frmt(0, 0, 1920, 1080,"HD",1.0);
    _blankViewerInfos.setRoD(RectI(0, 0, 1920, 1080));
    _blankViewerInfos.setDisplayWindow(frmt);
    setRod(_blankViewerInfos.getRoD());
    onProjectFormatChanged(frmt);
    _displayingImage = false;
    setMouseTracking(true);
    _ms = UNDEFINED;
    shaderLC = NULL;
    shaderRGB = NULL;
    shaderBlack = NULL;
    _overlay = true;
    _defaultDisplayTexture = 0;
    _displayChannels = 0.f;
    _progressBarY = -1;
    _drawProgressBar = false;
    _updatingTexture = false;
    populateMenu();
    // initTextFont();
    _clipToDisplayWindow = true;
    _displayPersistentMessage = false;
    _textRenderingColor.setRgba(qRgba(200,200,200,255));
    _displayWindowOverlayColor.setRgba(qRgba(125,125,125,255));
    _rodOverlayColor.setRgba(qRgba(100,100,100,255));
    _textFont = new QFont("Helvetica",15);
}

//void ViewerGL::initTextFont(){
//    QFile font(":/Resources/fonts/DejaVuSans.ttf");
//    uchar* buf = font.map(0,font.size());
//    _font = new FTTextureFont(buf,font.size());
//    if(_font->Error())
//        cout << "Failed to load the OpenGL text renderer font. " << endl;
//    else
//        _font->FaceSize(14);
//}
ViewerGL::ViewerGL(QGLContext* context,ViewerTab* parent,const QGLWidget* shareWidget)
    : QGLWidget(context,parent,shareWidget)
    , _shaderLoaded(false)
    , _viewerTab(parent)
    , _displayingImage(false)
    , _must_initBlackTex(true)
    , _clearColor(0,0,0,255)
    , _menu(new QMenu(this))
{
    initConstructor();
}

ViewerGL::ViewerGL(const QGLFormat& format,ViewerTab* parent ,const QGLWidget* shareWidget)
    : QGLWidget(format,parent,shareWidget)
    , _shaderLoaded(false)
    , _viewerTab(parent)
    , _displayingImage(false)
    , _must_initBlackTex(true)
    , _clearColor(0,0,0,255)
    , _menu(new QMenu(this))
{
    initConstructor();
}

ViewerGL::ViewerGL(ViewerTab* parent,const QGLWidget* shareWidget)
    : QGLWidget(parent,shareWidget)
    , _shaderLoaded(false)
    , _viewerTab(parent)
    , _displayingImage(false)
    , _must_initBlackTex(true)
    , _clearColor(0,0,0,255)
    , _menu(new QMenu(this))
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
    for(U32 i = 0; i < _pboIds.size();++i){
        glDeleteBuffers(1,&_pboIds[i]);
    }
    glDeleteBuffers(1, &_vboVerticesId);
    glDeleteBuffers(1, &_vboTexturesId);
    glDeleteBuffers(1, &_iboTriangleStripId);
    checkGLErrors();
    delete _textFont;
}

QSize ViewerGL::sizeHint() const{
    return QSize(500,500);
}
void ViewerGL::resizeGL(int width, int height){
    if(height == 0)// prevent division by 0
        height=1;
    float ap = getDisplayWindow().getPixelAspect();
    if(ap > 1.f){
        glViewport (0, 0, (int)(width*ap), height);
    }else{
        glViewport (0, 0, width, (int)(height/ap));
    }
    checkGLErrors();
    _ms = UNDEFINED;
    assert(_viewerTab);
    ViewerInstance* viewer = _viewerTab->getInternalNode();
    assert(viewer);
    if (viewer->getUiContext()) {
        viewer->refreshAndContinueRender(true);
        updateGL();
    }
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
    if(left == right || top == bottom){
        clearColorBuffer(_clearColor.redF(),_clearColor.greenF(),_clearColor.blueF(),_clearColor.alphaF());
        return;
    }
    _zoomCtx._lastOrthoLeft = left;
    _zoomCtx._lastOrthoRight = right;
    _zoomCtx._lastOrthoBottom = bottom;
    _zoomCtx._lastOrthoTop = top;
    glOrtho(left, right, bottom, top, -1, 1);
    checkGLErrors();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    {
        QMutexLocker locker(&_textureMutex);
        glEnable (GL_TEXTURE_2D);
        if(_displayingImage){
            glBindTexture(GL_TEXTURE_2D, _defaultDisplayTexture->getTexID());
            // debug (so the OpenGL debugger can make a breakpoint here)
            // GLfloat d;
            //  glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
            activateShaderRGB();
            checkGLErrors();
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
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if(_displayingImage){
        if(_hasHW){
            shaderRGB->release();
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
    if(_displayPersistentMessage){
        drawPersistentMessage();
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
    const RectI& dispW = getDisplayWindow();
    
    if(_clipToDisplayWindow){
        renderText(dispW.right(),dispW.bottom(), _resolutionOverlay,_textRenderingColor,*_textFont);
        
        QPoint topRight(dispW.right(),dispW.top());
        QPoint topLeft(dispW.left(),dispW.top());
        QPoint btmLeft(dispW.left(),dispW.bottom());
        QPoint btmRight(dispW.right(),dispW.bottom() );
        
        glBegin(GL_LINES);

        glColor4f( _displayWindowOverlayColor.redF(),
                   _displayWindowOverlayColor.greenF(),
                   _displayWindowOverlayColor.blueF(),
                   _displayWindowOverlayColor.alphaF() );
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
    }
    const RectI& dataW = getRoD();
    if((dispW != dataW && _clipToDisplayWindow) || !_clipToDisplayWindow){
        
        renderText(dataW.right(), dataW.top(), _topRightBBOXoverlay, _rodOverlayColor,*_textFont);
        renderText(dataW.left(), dataW.bottom(), _btmLeftBBOXoverlay, _rodOverlayColor,*_textFont);
        
        
        QPoint topRight2(dataW.right(), dataW.top());
        QPoint topLeft2(dataW.left(),dataW.top());
        QPoint btmLeft2(dataW.left(),dataW.bottom() );
        QPoint btmRight2(dataW.right(),dataW.bottom() );
        glPushAttrib(GL_LINE_BIT);
        glLineStipple(2, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);
        glBegin(GL_LINES);
        glColor4f( _rodOverlayColor.redF(),
                  _rodOverlayColor.greenF(),
                  _rodOverlayColor.blueF(),
                  _rodOverlayColor.alphaF() );
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
    if(_displayingImage){
        _viewerTab->getInternalNode()->drawOverlays();
    }
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
    checkGLErrors();
}
void ViewerGL::drawProgressBar(){
    const Format& dW = getDisplayWindow();
    glLineWidth(5);
    glBegin(GL_LINES);
    
    glVertex3f(dW.left(),_progressBarY,1);
    glVertex3f(dW.right(),_progressBarY,1);
    
    glEnd();
    glLineWidth(1);
    checkGLErrors();
}

void ViewerGL::drawPersistentMessage(){
    QFontMetrics metrics(font());
    int numberOfLines = std::ceil((double)metrics.width(_persistentMessage)/(double)(width()-20));
    int averageCharsPerLine = numberOfLines != 0 ? _persistentMessage.size() / numberOfLines : _persistentMessage.size();
    QStringList lines;
   
    int i = 0;
    while(i < _persistentMessage.size()) {
        QString str;
        while(i < _persistentMessage.size()){
            if(i%averageCharsPerLine == 0 && i!=0){
                break;
            }
            str.append(_persistentMessage.at(i));
            ++i;
        }
        /*Find closest word end and insert a new line*/
        while(i < _persistentMessage.size() && _persistentMessage.at(i)!=QChar(' ')){
            str.append(_persistentMessage.at(i));
            ++i;
        }
        lines.append(str);
    }
    
    
    QPointF topLeft = toImgCoordinates_fast(0,0);
    QPointF bottomRight = toImgCoordinates_fast(width(),numberOfLines*(metrics.height()*2));
    
    if(_persistentMessageType == 1){ // error
        glColor4f(0.5,0.,0.,1.);
    }else{ // warning
        glColor4f(0.65,0.65,0.,1.);
    }
    glBegin(GL_POLYGON);
    glVertex2f(topLeft.x(),topLeft.y()); //top left
    glVertex2f(topLeft.x(),bottomRight.y()); //bottom left
    glVertex2f(bottomRight.x(),bottomRight.y());//bottom right
    glVertex2f(bottomRight.x(),topLeft.y()); //top right
    glEnd();
    
    
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
    
    int offset = metrics.height()+10;
    for(int j = 0 ; j < lines.size();++j){
        QPointF pos = toImgCoordinates_fast(20, offset);
        renderText(pos.x(),pos.y(), lines.at(j),_textRenderingColor,*_textFont);
        offset += metrics.height()*2;
    }
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
    checkGLErrors();
}



void ViewerGL::initializeGL(){
    initAndCheckGlExtensions();
    _blackTex = new Texture;
    _defaultDisplayTexture = new Texture;
    
    
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

GLuint ViewerGL::getPboID(int index){
    if(index >= (int)_pboIds.size()){
        GLuint handle;
        glGenBuffersARB(1,&handle);
        _pboIds.push_back(handle);
        return handle;
    }else{
        return _pboIds[index];
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

std::pair<int,int> ViewerGL::computeRowSpan(int bottom,int top, std::vector<int>* rows) {
    /*First off,we test the 1st and last row to check wether the
     image is contained in the viewer*/
    // testing top of the image
    assert(rows);
    assert(rows->size() == 0);
    std::pair<int,int> ret;
    int y = 0;
    int prev = -1;
    double res = toImgCoordinates_fast(0,y).y();
    ret.first = bottom;
    ret.second = top-1;
    if (res < 0.) { // all the image is above the viewer
        return ret; // do not add any row
    }
    // testing bottom now
    y = height()-1;
    res = toImgCoordinates_fast(0,y).y();
    /*for all the others row (apart the first and last) we can check.
     */
    while(y >= 0 && res < bottom){
        /*while y is an invalid line, iterate*/
        --y;
        res = toImgCoordinates_fast(0,y).y();
    }
    while(y >= 0 && res >= bottom && res < top){
        /*y is a valid line in widget coord && res contains the image y coord.*/
        int row = (int)std::floor(res);
        assert(row >= bottom && row < top);
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
    assert(ret.first >= bottom && ret.first <= std::max(bottom, top-1));
    assert(ret.second >= std::min(bottom,top-1) && ret.second < top);
    return ret;
}

std::pair<int,int> ViewerGL::computeColumnSpan(int left,int right, std::vector<int>* columns) {
    /*First off,we test the 1st and last columns to check wether the
     image is contained in the viewer*/
    // testing right of the image
    assert(columns);
    assert(columns->size() == 0);
    std::pair<int,int> ret;
    int x = width()-1;
    int prev = -1;
    double res = toImgCoordinates_fast(x,0).x();
    ret.first = left;
    ret.second = right-1;
    if (res < 0.) { // all the image is on the left of the viewer
        return ret;
    }
    // testing right now
    x = 0;
    res = toImgCoordinates_fast(x,0).x();
    /*for all the others columns (apart the first and last) we can check.
     */
    while(x < width() && res < left) {
        /*while x is an invalid column, iterate from left to right*/
        ++x;
        res = toImgCoordinates_fast(x,0).x();
    }
    while(x < width() && res >= left && res < right) {
        /*y is a valid column in widget coord && res contains the image x coord.*/
        int column = (int)std::floor(res);
        assert(column >= left && column < right);
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
    assert(ret.first >= left && ret.first <= std::max(left, right-1));
    assert(ret.second >= std::min(left,right-1) && ret.second < right);
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


void ViewerGL::initAndCheckGlExtensions() {
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        Natron::errorDialog("OpenGL/GLEW error",
                             (const char*)glewGetErrorString(err));
    }
    //fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    // is GL_VERSION_2_0 necessary? note that GL_VERSION_2_0 includes GLSL
    if (!glewIsSupported("GL_VERSION_1_5 "
                         "GL_ARB_texture_non_power_of_two " // or GL_IMG_texture_npot, or GL_OES_texture_npot, core since 2.0
                         "GL_ARB_shader_objects " // GLSL, Uniform*, core since 2.0
                         "GL_ARB_vertex_buffer_object " // BindBuffer, MapBuffer, etc.
                         "GL_ARB_pixel_buffer_object " // BindBuffer(PIXEL_UNPACK_BUFFER,...
                         //"GL_ARB_vertex_array_object " // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                         //"GL_ARB_framebuffer_object " // or GL_EXT_framebuffer_object GenFramebuffers, core since version 3.0
                         )) {
        Natron::errorDialog("Missing OpenGL requirements",
                             "The viewer may not be fully functionnal. "
                             "This software needs at least OpenGL 1.5 with NPOT textures, GLSL, VBO, PBO, vertex arrays. ");
    }
    if (!QGLShaderProgram::hasOpenGLShaderPrograms(context())) {
        // no need to pull out a dialog, it was already presented after the GLEW check above

        //Natron::errorDialog("Viewer error","The viewer is unable to work without a proper version of GLSL.");
        //cout << "Warning : GLSL not present on this hardware, no material acceleration possible." << endl;
        _hasHW = false;
    }
}

void ViewerGL::activateShaderLC(){
    if(!_hasHW) return;
    if(!shaderLC->bind()){
        cout << qPrintable(shaderLC->log()) << endl;
    }
    shaderLC->setUniformValue("Tex", 0);
    shaderLC->setUniformValue("yw",1.0,1.0,1.0);
    shaderLC->setUniformValue("expMult",  (GLfloat)_viewerTab->getInternalNode()->getExposure());
    // FIXME: why a float to really represent an enum????
    shaderLC->setUniformValue("lut", (GLfloat)_viewerTab->getInternalNode()->getLutType());
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
    shaderRGB->setUniformValue("expMult",  (GLfloat)_viewerTab->getInternalNode()->getExposure());
    // FIXME-seeabove: why a float to really represent an enum????
    shaderRGB->setUniformValue("lut", (GLfloat)_viewerTab->getInternalNode()->getLutType());
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
    fitToFormat(getDisplayWindow());
    
    TextureRect texSize(0, 0, 2047, 1555,2048,1556);
    assert_checkGLErrors();
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, getPboID(0));
    checkGLErrors();
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texSize.w*texSize.h*sizeof(U32), NULL, GL_DYNAMIC_DRAW_ARB);
    checkGLErrors();
    char* frameData = (char*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    assert(frameData);
    U32* output = reinterpret_cast<U32*>(frameData);
    for(int i = 0 ; i < texSize.w*texSize.h ; ++i) {
        output[i] = ViewerInstance::toBGRA(0, 0, 0, 255);
    }
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
    checkGLErrors();
    _blackTex->fillOrAllocateTexture(texSize,Texture::BYTE);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    checkGLErrors();
    _must_initBlackTex = false;
}



void ViewerGL::transferBufferFromRAMtoGPU(const unsigned char* ramBuffer, size_t bytesCount, const TextureRect& region,int pboIndex){
    QMutexLocker locker(&_textureMutex);
    GLint currentBoundPBO;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    if (currentBoundPBO != 0) {
        cout << "(ViewerGL::allocateAndMapPBO): Another PBO is currently mapped, glMap failed." << endl;
        return;
    }
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, getPboID(pboIndex));
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);
    GLvoid *ret = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    assert(ret);
    
    memcpy(ret, (void*)ramBuffer, bytesCount);

    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    checkGLErrors();
    
    if(byteMode() == 1.f || !_hasHW){
        _defaultDisplayTexture->fillOrAllocateTexture(region,Texture::BYTE);
    }else{
        _defaultDisplayTexture->fillOrAllocateTexture(region,Texture::FLOAT);
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
    checkGLErrors();
}

#if QT_VERSION < 0x050000
#define QMouseEventLocalPos(e) (e->posF())
#else
#define QMouseEventLocalPos(e) (e->localPos())
#endif

void ViewerGL::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::RightButton){
        _menu->exec(mapToGlobal(event->pos()));
        return;
    }
    
    _zoomCtx._oldClick = event->pos();
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _ms = DRAGGING;
    } else if (event->button() == Qt::LeftButton) {
        _viewerTab->getInternalNode()->notifyOverlaysPenDown(QMouseEventLocalPos(event),
                                                             toImgCoordinates_fast(event->x(), event->y()));
    }
    QGLWidget::mousePressEvent(event);
}

void ViewerGL::mouseReleaseEvent(QMouseEvent *event){
    _ms = UNDEFINED;
    _viewerTab->getInternalNode()->notifyOverlaysPenUp(QMouseEventLocalPos(event),
                                                       toImgCoordinates_fast(event->x(), event->y()));
    QGLWidget::mouseReleaseEvent(event);
}
void ViewerGL::mouseMoveEvent(QMouseEvent *event) {
    QPointF pos = toImgCoordinates_fast(event->x(), event->y());
    const Format& dispW = getDisplayWindow();
    // if the mouse is inside the image, update the color picker
    if (pos.x() >= dispW.left() &&
            pos.x() <= dispW.width() &&
            pos.y() >= dispW.bottom() &&
            pos.y() <= dispW.height() &&
            event->x() >= 0 && event->x() < width() &&
            event->y() >= 0 && event->y() < height()) {
        if (!_infoViewer->colorAndMouseVisible()) {
            _infoViewer->showColorAndMouseInfo();
        }
        boost::shared_ptr<VideoEngine> videoEngine = _viewerTab->getInternalNode()->getVideoEngine();
        if (!videoEngine->isWorking()) {
            updateColorPicker(event->x(),event->y());
        }
        _infoViewer->setMousePos(QPoint((int)pos.x(),(int)pos.y()));
        emit infoMousePosChanged();
    } else {
        if(_infoViewer->colorAndMouseVisible()){
            _infoViewer->hideColorAndMouseInfo();
        }
    }
    
    
    if (_ms == DRAGGING) {
        QPoint newClick =  event->pos();
        QPointF newClick_opengl = toImgCoordinates_fast(newClick.x(),newClick.y());
        QPointF oldClick_opengl = toImgCoordinates_fast(_zoomCtx._oldClick.x(),_zoomCtx._oldClick.y());
        float dy = (oldClick_opengl.y() - newClick_opengl.y());
        _zoomCtx._bottom += dy;
        _zoomCtx._left += (oldClick_opengl.x() - newClick_opengl.x());
        _zoomCtx._oldClick = newClick;
        if(_displayingImage){
            _viewerTab->getInternalNode()->refreshAndContinueRender();
        }
        //else {
        updateGL();
        // }
        // no need to update the color picker or mouse posn: they should be unchanged
    } else {
        _viewerTab->getInternalNode()->notifyOverlaysPenMotion(QMouseEventLocalPos(event),pos);
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
    if (event->orientation() != Qt::Vertical) {
        return;
    }
    double newZoomFactor;
    if (event->delta() > 0) {
        newZoomFactor = _zoomCtx._zoomFactor*std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    } else {
        newZoomFactor = _zoomCtx._zoomFactor/std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, -event->delta());
    }
    if (newZoomFactor <= 0.01) {
        newZoomFactor = 0.01;
    } else if (newZoomFactor > 1024.) {
        newZoomFactor = 1024.;
    }
    QPointF zoomCenter = toImgCoordinates_fast(event->x(), event->y());
    double zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
    _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio;
    _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    if(_displayingImage){
        appPTR->clearPlaybackCache();
        _viewerTab->getInternalNode()->refreshAndContinueRender();
    }
    //else {
    updateGL();
    //}
    
    assert(0 < _zoomCtx._zoomFactor && _zoomCtx._zoomFactor <= 1024);
    int zoomValue = (int)(100*_zoomCtx._zoomFactor);
    if (zoomValue == 0) {
        zoomValue = 1; // sometimes, floor(100*0.01) makes 0
    }
    assert(zoomValue > 0);
    emit zoomChanged(zoomValue);
}

void ViewerGL::zoomSlot(int v) {
    assert(v > 0);
    double newZoomFactor = v/100.;
    if(newZoomFactor < 0.01) {
        newZoomFactor = 0.01;
    } else if (newZoomFactor > 1024.) {
        newZoomFactor = 1024.;
    }
    double zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
    double w = (double)width();
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / (double)_zoomCtx._zoomFactor;
    double right = left +  w / (double)_zoomCtx._zoomFactor;
    
    _zoomCtx._left = (right + left)/2. - zoomRatio * (right - left)/2.;
    _zoomCtx._bottom = (top + bottom)/2. - zoomRatio * (top - bottom)/2.;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    if(_displayingImage){
        appPTR->clearPlaybackCache();
        _viewerTab->getInternalNode()->refreshAndContinueRender();
    } else {
        updateGL();
    }
    assert(0 < _zoomCtx._zoomFactor && _zoomCtx._zoomFactor <= 1024);
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

#if 0
/*Returns coordinates with 0,0 at top left, Natron inverts
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
#endif

QVector4D ViewerGL::getColorUnderMouse(int x,int y){
    
    QPointF pos = toImgCoordinates_fast(x, y);
    if(pos.x() < getDisplayWindow().left() || pos.x() >= getDisplayWindow().width() || pos.y() < getDisplayWindow().bottom() || pos.y() >=getDisplayWindow().height())
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
        return QVector4D((float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f);
    }else if(byteMode()==0 && _hasHW){
        GLfloat pixel[4];
        glReadPixels( x, height()-y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        checkGLErrors();
        return QVector4D(pixel[0],pixel[1],pixel[2],pixel[3]);
    }
    return QVector4D(0,0,0,0);
}

void ViewerGL::fitToFormat(const RectI& rod){
    double h = rod.height();
    double w = rod.width();
    assert(h > 0. && w > 0.);
    double zoomFactor = height()/h;
    zoomFactor = (zoomFactor > 0.06) ? (zoomFactor-0.05) : std::max(zoomFactor,0.01);
    assert(zoomFactor>=0.01 && zoomFactor <= 1024);
    _zoomCtx.setZoomFactor(zoomFactor);
    emit zoomChanged(zoomFactor * 100);
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




void ViewerGL::disconnectViewer(){
    _viewerTab->getInternalNode()->getVideoEngine()->abortRendering(); // aborting current work
    setRod(_blankViewerInfos.getRoD());
    fitToFormat(getDisplayWindow());
    clearViewer();
}



/*The dataWindow of the currentFrame(BBOX)*/
const RectI& ViewerGL::getRoD() const {return _currentViewerInfos.getRoD();}

/*The displayWindow of the currentFrame(Resolution)*/
const Format& ViewerGL::getDisplayWindow() const {return _currentViewerInfos.getDisplayWindow();}


void ViewerGL::setRod(const RectI& rod){

    _currentViewerInfos.setRoD(rod);
    emit infoDataWindowChanged();
    _btmLeftBBOXoverlay.clear();
    _btmLeftBBOXoverlay.append(QString::number(rod.left()));
    _btmLeftBBOXoverlay.append(",");
    _btmLeftBBOXoverlay.append(QString::number(rod.bottom()));
    _topRightBBOXoverlay.clear();
    _topRightBBOXoverlay.append(QString::number(rod.right()));
    _topRightBBOXoverlay.append(",");
    _topRightBBOXoverlay.append(QString::number(rod.top()));
    
    
    
}
void ViewerGL::onProjectFormatChanged(const Format& format){
    _currentViewerInfos.setDisplayWindow(format);
    _blankViewerInfos.setDisplayWindow(format);
    _blankViewerInfos.setRoD(format);
    emit infoResolutionChanged();
    _resolutionOverlay.clear();
    _resolutionOverlay.append(QString::number(format.width()));
    _resolutionOverlay.append("x");
    _resolutionOverlay.append(QString::number(format.height()));

}

void ViewerGL::setClipToDisplayWindow(bool b) {
    _clipToDisplayWindow = b;
    ViewerInstance* viewer = _viewerTab->getInternalNode();
    assert(viewer);
    if (viewer->getUiContext()) {
        _viewerTab->abortRendering();
        _viewerTab->getInternalNode()->refreshAndContinueRender(false);
    }
}

/*display black in the viewer*/
void ViewerGL::clearViewer(){
    setDisplayingImage(false);
    updateGL();
}

/*overload of QT enter/leave/resize events*/
void ViewerGL::enterEvent(QEvent *event)
{   QGLWidget::enterEvent(event);
    //    _viewerTab->getInternalNode()->notifyOverlaysFocusGained();
    
}
void ViewerGL::leaveEvent(QEvent *event)
{
    QGLWidget::leaveEvent(event);
    // _viewerTab->getInternalNode()->notifyOverlaysFocusLost();
    
}
void ViewerGL::resizeEvent(QResizeEvent* event){ // public to hack the protected field
    QGLWidget::resizeEvent(event);
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
    return appPTR->getCurrentSettings()._viewerSettings.byte_mode;
}

void ViewerGL::setDisplayChannel(const ChannelSet& channels,bool yMode){
    if(yMode){
        _displayChannels = 5.f;
        
    }else{
        if(channels == Natron::Mask_RGB || channels == Natron::Mask_RGBA)
            _displayChannels = 0.f;
        else if((channels & Natron::Channel_red) == Natron::Channel_red)
            _displayChannels = 1.f;
        else if((channels & Natron::Channel_green) == Natron::Channel_green)
            _displayChannels = 2.f;
        else if((channels & Natron::Channel_blue) == Natron::Channel_blue)
            _displayChannels = 3.f;
        else if((channels & Natron::Channel_alpha) == Natron::Channel_alpha)
            _displayChannels = 4.f;
        
    }
    updateGL();
    
}
void ViewerGL::updateProgressOnViewer(const TextureRect& /*region*/,int /*y*/ , int /*texY*/) {
//    _updatingTexture = true;
//    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
//    checkGLErrors();
//    if(byteMode() == 1.f || !_hasHW){
//        _defaultDisplayTexture->updatePartOfTexture(region,texY,Texture::BYTE);
//    }else{
//        _defaultDisplayTexture->updatePartOfTexture(region,texY,Texture::FLOAT);
//    }
//    _drawProgressBar = true;
//    _progressBarY = y;
//    updateGL();
//    assert(!frameData);
//    frameData = (char*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
//    checkGLErrors();
//
//    _updatingTexture = false;
}

void ViewerGL::doSwapBuffers(){
    swapBuffers();
}
void ViewerGL::populateMenu(){
    _menu->clear();
    QAction* displayOverlaysAction = new QAction("Display overlays",this);
    displayOverlaysAction->setCheckable(true);
    displayOverlaysAction->setChecked(true);
    QObject::connect(displayOverlaysAction,SIGNAL(triggered()),this,SLOT(toggleOverlays()));
    _menu->addAction(displayOverlaysAction);
}

void ViewerGL::renderText( int x, int y, const QString &string,const QColor& color,const QFont& font)
{
    
    if(string.isEmpty())
        return;
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    QPointF pos = toWidgetCoordinates(x, y);
    _textRenderer.renderText(pos.x(),h-pos.y(),string,color,font);
    checkGLErrors();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(_zoomCtx._lastOrthoLeft,_zoomCtx._lastOrthoRight,_zoomCtx._lastOrthoBottom,_zoomCtx._lastOrthoTop,-1,1);
    glMatrixMode(GL_MODELVIEW);
}

void ViewerGL::setPersistentMessage(int type,const QString& message){
    _persistentMessageType = type;
    _persistentMessage = message;
    _displayPersistentMessage = true;
    updateGL();
}

void ViewerGL::clearPersistentMessage(){
    if(!_displayPersistentMessage){
        return;
    }
    _displayPersistentMessage = false;
    updateGL();
}


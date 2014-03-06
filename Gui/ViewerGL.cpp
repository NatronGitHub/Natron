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

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QMenu> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QDockWidget> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QtGui/QPainter>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QVector4D>
#include <QtOpenGL/QGLShaderProgram>

#include "Global/Macros.h"
GCC_DIAG_OFF(unused-parameter);
GCC_DIAG_ON(unused-parameter);


#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/ImageInfo.h"
#include "Engine/Lut.h"
#include "Engine/MemoryFile.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/Texture.h"
#include "Gui/Shaders.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TextRenderer.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ProjectGui.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

#define USER_ROI_BORDER_TICK_SIZE 15.f
#define USER_ROI_CROSS_RADIUS 15.f
#define USER_ROI_SELECTION_POINT_SIZE 8.f
#define USER_ROI_CLICK_TOLERANCE 8.f

/*This class is the the core of the viewer : what displays images, overlays, etc...
 Everything related to OpenGL will (almost always) be in this class */

//using namespace Imf;
//using namespace Imath;
using namespace Natron;
using std::cout; using std::endl;

namespace {
/**
 *@class ZoomContext
 *@brief Holds all zoom related variables. This is an internal class used by the ViewerGL.
 *The variables stored here are the minimal variables needed to enable the zoom and the drag
 *of the image.
 *The top and right edges of the ortographic projection can be computed as such:
 *
 * top = bottom + heightWidget/zoomFactor
 * right = left + widthWidget/zoomFactor
 *
 *
 *During the computations made in the ViewerGL, we define 2 coordinate systems:
 *  - The viewport (or widget) coordinate system, with origin top left.
 *  - The image coordinate system with the origin bottom left.
 *To transform the coordinates between one system to another is a simple mapping operation,
 *which yields :
 *
 * Ximg = (Xwidget/widthWidget) * ( right - left ) + left
 * Yimg = (Ywidget/heightWidget) * (bottom - top) + top  [notice the y inversion here]
 *
 *Let us define the zoomFactor being the ratio of screen pixels size divided by image pixels size.
 *
 *Zooming to a point is simply a matter of changing the orthographic projection.
 *When zooming, the position of the center should never change, relativly to the orthographic projection.
 *Which means that the old position (before zooming) expressed in its own orthographic projection, should equal
 *the new position (after zooming) expressed in its own orthographic projection.
 *That is:
 *
 * - For the x coordinate:  (Ximg - left_old) * zoomFactor_old == (Ximg - left_new) * zoomFactor_new
 *Where Ximg is the X coordinate of the zoom center in image coordinates, left_old is the left edge of
 *the orthographic projection before zooming, and the left_new the left edge after zooming.
 *
 *This formula yields:
 *
 *  left_new = Ximg - (Ximg - left_old)*(zoomFactor_old/zoomFactor_new).
 *
 * -The y coordinate follows exactly the same reasoning and the following equation can be found:
 *
 *  bottom_new = Yimg - (Yimg - bottom_old)*(zoomFactor_old/zoomFactor_new).
 *
 * Retrieving top_new and right_new can be done with the formulas exhibited above.
 *
 *A last note on the zoom is the initialisation. A desired effect can be to initialise the image
 *so it appears centered in the viewer and that fit entirely in the viewer. This can be done as such:
 *
 *The zoomFactor needed to fit the image in the viewer can be computed with the ratio of the
 *height of the widget by the height of the image :
 *
 * zoomFactor = heightWidget / heightImage
 *
 *The left and bottom edges can then be initialised :
 *
 * left = widthImage / 2 - ( widthWidget / ( 2 * zoomFactor ) )
 * bottom = heightImage / 2 - ( heightWidget / ( 2 * zoomFactor ) )
 *
 *TRANSLATING THE IMAGE : (panning around)
 *
 *Translation is simply a matter of displacing the edges of the orthographic projection
 *by a delta. The delta is the difference between the last mouse position (in image coordinates)
 *and the new mouse position (in image coordinates).
 *
 *Translating is just doing so:
 *
 *  bottom += Yimg_old - Yimg_new
 *  left += Ximg_old - Ximg_new
 **/
struct ZoomContext{

    ZoomContext()
    : bottom(0.)
    , left(0.)
    , zoomFactor(1.)
    {}

    QMutex zoomContextLock;
    QPoint oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    double bottom; /// the bottom edge of orthographic projection
    double left; /// the left edge of the orthographic projection
    double zoomFactor; /// the zoom factor applied to the current image

    double lastOrthoLeft, lastOrthoBottom, lastOrthoRight, lastOrthoTop; //< remembers the last values passed to the glOrtho call
};

/**
 *@enum MOUSE_STATE
 *@brief basic state switching for mouse events
 **/
enum MOUSE_STATE{
    DRAGGING_IMAGE = 0,
    DRAGGING_ROI_LEFT_EDGE,
    DRAGGING_ROI_RIGHT_EDGE,
    DRAGGING_ROI_TOP_EDGE,
    DRAGGING_ROI_BOTTOM_EDGE,
    DRAGGING_ROI_TOP_LEFT,
    DRAGGING_ROI_TOP_RIGHT,
    DRAGGING_ROI_BOTTOM_RIGHT,
    DRAGGING_ROI_BOTTOM_LEFT,
    DRAGGING_ROI_CROSS,
    UNDEFINED};
} // namespace

struct ViewerGL::Implementation {

    Implementation(ViewerTab* parent, ViewerGL* _this)
    : pboIds()
    , vboVerticesId(0)
    , vboTexturesId(0)
    , iboTriangleStripId(0)
    , defaultDisplayTexture(0)
    , textureMutex()
    , blackTex(0)
    , shaderRGB(0)
    , shaderBlack(0)
    , shaderLoaded(false)
    , infoViewer(0)
    , viewerTab(parent)
    , currentViewerInfos()
    , blankViewerInfos()
    , displayingImage(false)
    , must_initBlackTex(true)
    , ms(UNDEFINED)
    , zoomCtx()
    , resolutionOverlay()
    , btmLeftBBOXoverlay()
    , topRightBBOXoverlay()
    , textRenderingColor(200,200,200,255)
    , displayWindowOverlayColor(125,125,125,255)
    , rodOverlayColor(100,100,100,255)
    , textFont(new QFont(NATRON_FONT, NATRON_FONT_SIZE_13))
    , overlay(true)
    , supportsGLSL(true)
    , updatingTexture(false)
    , clearColor(0,0,0,255)
    , menu(new QMenu(_this))
    , clipToDisplayWindow(true)
    , persistentMessage()
    , persistentMessageType(0)
    , displayPersistentMessage(false)
    , zoomOrPannedSinceLastFit(false)
    , textRenderer()
    , userRoI()
    , isUserRoISet(false)
    , isUserRoIEnabled(false)
    , lastMousePosition()
    {
    }
    
    std::vector<GLuint> pboIds; /*!< PBO's id's used by the OpenGL context*/

    //   GLuint vaoId; /*!< VAO holding the rendering VBOs for texture mapping.*/

    GLuint vboVerticesId; /*!< VBO holding the vertices for the texture mapping*/

    GLuint vboTexturesId; /*!< VBO holding texture coordinates*/

    GLuint iboTriangleStripId; /*!< IBOs holding vertices indexes for triangle strip sets*/

    Texture* defaultDisplayTexture;/*!< A pointer to the current texture used to display.*/
    QMutex textureMutex;/*!< protects defaultDisplayTexture*/

    Texture* blackTex;/*!< the texture used to render a black screen when nothing is connected.*/

    QGLShaderProgram* shaderRGB;/*!< The shader program used to render RGB data*/
    QGLShaderProgram* shaderBlack;/*!< The shader program used when the viewer is disconnected.*/

    bool shaderLoaded;/*!< Flag to check whether the shaders have already been loaded.*/


    InfoViewerWidget* infoViewer;/*!< Pointer to the info bar below the viewer holding pixel/mouse/format related infos*/

    ViewerTab* viewerTab;/*!< Pointer to the viewer tab GUI*/

    QMutex currentViewerInfosLock;
    ImageInfo currentViewerInfos;/*!< Pointer to the ViewerInfos  used for rendering*/

    ImageInfo blankViewerInfos;/*!< Pointer to the infos used when the viewer is disconnected.*/

    bool displayingImage;/*!< True if the viewer is connected and not displaying black.*/
    bool must_initBlackTex;

    MOUSE_STATE ms;/*!< Holds the mouse state*/

    ZoomContext zoomCtx;/*!< All zoom related variables are packed into this object*/

    QString resolutionOverlay;/*!< The string holding the resolution overlay, e.g: "1920x1080"*/
    QString btmLeftBBOXoverlay;/*!< The string holding the bottom left corner coordinates of the dataWindow*/
    QString topRightBBOXoverlay;/*!< The string holding the top right corner coordinates of the dataWindow*/
    const QColor textRenderingColor;
    const QColor displayWindowOverlayColor;
    const QColor rodOverlayColor;
    QFont* textFont;

    bool overlay;/*!< True if the user enabled overlay dispay*/

    bool supportsGLSL;/*!< True if the user has a GLSL version supporting everything requested.*/

    bool updatingTexture;

    QColor clearColor;

    QMenu* menu;

    QMutex clipToDisplayWindowMutex;
    bool clipToDisplayWindow;

    QString persistentMessage;
    int persistentMessageType;
    bool displayPersistentMessage;

    bool zoomOrPannedSinceLastFit; //< true if the user zoomed or panned the image since the last call to fitToRoD
    
    Natron::TextRenderer textRenderer;
    
    QMutex userRoIMutex;
    RectI userRoI;
    bool isUserRoISet;
    bool isUserRoIEnabled;
    QPoint lastMousePosition;
};

//static const GLfloat renderingTextureCoordinates[32] = {
//    0 , 1 , //0
//    0 , 1 , //1
//    1 , 1 ,//2
//    1 , 1 , //3
//    0 , 1 , //4
//    0 , 1 , //5
//    1 , 1 , //6
//    1 , 1 , //7
//    0 , 0 , //8
//    0 , 0 , //9
//    1 , 0 ,  //10
//    1 , 0 , //11
//    0 , 0 , // 12
//    0 , 0 , //13
//    1 , 0 , //14
//    1 , 0   //15
//};

/*see http://www.learnopengles.com/android-lesson-eight-an-introduction-to-index-buffer-objects-ibos/ */
static const GLubyte triangleStrip[28] = {0,4,1,5,2,6,3,7,
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
void ViewerGL::drawRenderingVAO() {
    assert(QGLContext::currentContext() == context());
    
    ///the texture rectangle in image coordinates. The values in it are multiples of tile size.
    ///
    const TextureRect &r = _imp->displayingImage ? _imp->defaultDisplayTexture->getTextureRect() : _imp->blackTex->getTextureRect();
    
    ///the RoD of the iamge
    RectI rod = _imp->clipToDisplayWindow ? getDisplayWindow() : getRoD();

    ///clip the RoD to the portion where data lies. (i.e: r might be smaller than rod when it is the project window.)
    ///if so then we don't want to display "all" the project window.
    if (_imp->clipToDisplayWindow) {
        rod.intersect(getRoD(), &rod);
    }
    
    //if user RoI is enabled, clip the rod to that roi
    if (_imp->isUserRoIEnabled) {
        //if the userRoI isn't intersecting the rod, just don't render anything
        if(!rod.intersect(_imp->userRoI,&rod)) {
            return;
        }
    }
   
    /*setup the scissor box to paint only what's contained in the project's window*/
    QPointF scissorBoxBtmLeft = toWidgetCoordinates(rod.x1, rod.y1);
    QPointF scissorBoxTopRight = toWidgetCoordinates(rod.x2, rod.y2);
    
    /*invert y coordinate as OpenGL expects btm left corner to be 0,0*/
    scissorBoxBtmLeft.ry() = height() - scissorBoxBtmLeft.ry();
    scissorBoxTopRight.ry() = height() - scissorBoxTopRight.ry();
    
    glScissor(scissorBoxBtmLeft.x(),scissorBoxBtmLeft.y(),scissorBoxTopRight.x() - scissorBoxBtmLeft.x(),
              scissorBoxTopRight.y() - scissorBoxBtmLeft.y());
    
    GLfloat vertices[32] = {
        (GLfloat)rod.left() ,(GLfloat)rod.top()  , //0
        (GLfloat)r.x1       , (GLfloat)rod.top()  , //1
        (GLfloat)r.x2 , (GLfloat)rod.top()  , //2
        (GLfloat)rod.right(),(GLfloat)rod.top()  , //3
        (GLfloat)rod.left(), (GLfloat)r.y2, //4
        (GLfloat)r.x1      ,  (GLfloat)r.y2, //5
        (GLfloat)r.x2,  (GLfloat)r.y2, //6
        (GLfloat)rod.right(),(GLfloat)r.y2, //7
        (GLfloat)rod.left() ,(GLfloat)r.y1      , //8
        (GLfloat)r.x1      ,  (GLfloat)r.y1      , //9
        (GLfloat)r.x2,  (GLfloat)r.y1      , //10
        (GLfloat)rod.right(),(GLfloat)r.y1      , //11
        (GLfloat)rod.left(), (GLfloat)rod.bottom(), //12
        (GLfloat)r.x1      ,  (GLfloat)rod.bottom(), //13
        (GLfloat)r.x2,  (GLfloat)rod.bottom(), //14
        (GLfloat)rod.right(),(GLfloat)rod.bottom() //15
    };

   // std::cout << "ViewerGL: x1= " << r.x1 << " x2= " << r.x2 << " y1= " << r.y1 << " y2= " << r.y2 << std::endl;

    GLfloat texBottom,texLeft,texRight,texTop;
    texBottom =  0;
    texTop =  (GLfloat)(r.y2 - r.y1)/ (GLfloat)(r.h * r.closestPo2);
    texLeft = 0;
    texRight = (GLfloat)(r.x2 - r.x1) / (GLfloat)(r.w * r.closestPo2);
    
    // texTop = texTop > 1 ? 1 : texTop;
    //  texRight = texRight > 1 ? 1 : texRight;
    
    
    GLfloat renderingTextureCoordinates[32] = {
        texLeft , texTop , //0
        texLeft , texTop , //1
        texRight , texTop ,//2
        texRight , texTop , //3
        texLeft , texTop , //4
        texLeft , texTop , //5
        texRight , texTop , //6
        texRight , texTop , //7
        texLeft , texBottom , //8
        texLeft , texBottom , //9
        texRight , texBottom ,  //10
        texRight , texBottom , //11
        texLeft , texBottom , // 12
        texLeft , texBottom , //13
        texRight , texBottom , //14
        texRight , texBottom   //15
    };
    
    glEnable(GL_SCISSOR_TEST);

    
    glBindBuffer(GL_ARRAY_BUFFER, _imp->vboVerticesId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 32*sizeof(GLfloat), vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, _imp->vboTexturesId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 32*sizeof(GLfloat), renderingTextureCoordinates);
    glClientActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0 , 0);
    
    glBindBuffer(GL_ARRAY_BUFFER,0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _imp->iboTriangleStripId);
    glDrawElements(GL_TRIANGLE_STRIP, 28, GL_UNSIGNED_BYTE, 0);
    checkGLErrors();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    checkGLErrors();
    
    glDisable(GL_SCISSOR_TEST);
}

#if 0
void ViewerGL::checkFrameBufferCompleteness(const char where[],bool silent)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
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

ViewerGL::ViewerGL(ViewerTab* parent,const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _imp(new Implementation(parent, this))
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    QObject::connect(parent->getGui()->getApp()->getProject().get(),SIGNAL(formatChanged(Format)),this,SLOT(onProjectFormatChanged(Format)));
    
    populateMenu();

    
    _imp->blankViewerInfos.setChannels(Natron::Mask_RGBA);
    const Format& projectFormat = parent->getGui()->getApp()->getProject()->getProjectDefaultFormat();
    _imp->blankViewerInfos.setRoD(projectFormat);
    _imp->blankViewerInfos.setDisplayWindow(projectFormat);
    setRegionOfDefinition(_imp->blankViewerInfos.getRoD());
    onProjectFormatChanged(projectFormat);

}


ViewerGL::~ViewerGL()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(_imp->shaderRGB){
        _imp->shaderRGB->removeAllShaders();
        delete _imp->shaderRGB;
    }
    if(_imp->shaderBlack){
        _imp->shaderBlack->removeAllShaders();
        delete _imp->shaderBlack;
        
    }
    delete _imp->blackTex;
    delete _imp->defaultDisplayTexture;
    for(U32 i = 0; i < _imp->pboIds.size();++i){
        glDeleteBuffers(1,&_imp->pboIds[i]);
    }
    glDeleteBuffers(1, &_imp->vboVerticesId);
    glDeleteBuffers(1, &_imp->vboTexturesId);
    glDeleteBuffers(1, &_imp->iboTriangleStripId);
    checkGLErrors();
    delete _imp->textFont;
}

QSize ViewerGL::sizeHint() const
{
    return QSize(1000,1000);
}

const QFont& ViewerGL::textFont() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return *_imp->textFont;
}

void ViewerGL::setTextFont(const QFont& f)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    *_imp->textFont = f;
}

/**
 *@brief Toggles on/off the display on the viewer. If d is false then it will
 *render black only.
 **/
void ViewerGL::setDisplayingImage(bool d)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->displayingImage = d;
    if (!_imp->displayingImage) {
        _imp->must_initBlackTex = true;
    };
}

/**
 *@returns Returns true if the viewer is displaying something.
 **/
bool ViewerGL::displayingImage() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->displayingImage;
}

void ViewerGL::resizeGL(int width, int height)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(height == 0)// prevent division by 0
        height=1;
    float ap = getDisplayWindow().getPixelAspect();
    if(ap > 1.f){
        glViewport (0, 0, (int)(width*ap), height);
    }else{
        glViewport (0, 0, width, (int)(height/ap));
    }
    checkGLErrors();
    _imp->ms = UNDEFINED;
    assert(_imp->viewerTab);
    ViewerInstance* viewer = _imp->viewerTab->getInternalNode();
    assert(viewer);
    if (!_imp->displayingImage && !_imp->zoomOrPannedSinceLastFit) {
        fitImageToFormat(getDisplayWindow());
    }
    if (viewer->getUiContext() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject()) {
        viewer->refreshAndContinueRender(!_imp->zoomOrPannedSinceLastFit,false);
        updateGL();
    }
}

void ViewerGL::paintGL()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    checkGLErrors();
    if (_imp->must_initBlackTex) {
        initBlackTex();
    }
    double w = (double)width();
    double h = (double)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    assert(_imp->zoomCtx.zoomFactor > 0);
    assert(_imp->zoomCtx.zoomFactor <= 1024);
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / (double)_imp->zoomCtx.zoomFactor * _imp->currentViewerInfos.getDisplayWindow().getPixelAspect();
    double right = left +  w / (double)_imp->zoomCtx.zoomFactor ;
    if(left == right || top == bottom){
        clearColorBuffer(_imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF());
        return;
    }
    _imp->zoomCtx.lastOrthoLeft = left;
    _imp->zoomCtx.lastOrthoRight = right;
    _imp->zoomCtx.lastOrthoBottom = bottom;
    _imp->zoomCtx.lastOrthoTop = top;
    glOrtho(left, right, bottom, top, -1, 1);
    checkGLErrors();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    {
        QMutexLocker locker(&_imp->textureMutex);
        glEnable (GL_TEXTURE_2D);
        if(_imp->displayingImage){
            glBindTexture(GL_TEXTURE_2D, _imp->defaultDisplayTexture->getTexID());
            // debug (so the OpenGL debugger can make a breakpoint here)
            // GLfloat d;
            //  glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
            if (_imp->supportsGLSL) {
                activateShaderRGB();
            }
            checkGLErrors();
        }else{
            glBindTexture(GL_TEXTURE_2D, _imp->blackTex->getTexID());
            checkGLErrors();
            if(_imp->supportsGLSL && !_imp->shaderBlack->bind()){
                cout << qPrintable(_imp->shaderBlack->log()) << endl;
                checkGLErrors();
            }
            if(_imp->supportsGLSL)
                _imp->shaderBlack->setUniformValue("Tex", 0);
            checkGLErrors();
            
        }

        clearColorBuffer(_imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF());
        drawRenderingVAO();
        checkGLErrors();
    }
    
    if (_imp->displayingImage) {
        if (_imp->supportsGLSL) {
            _imp->shaderRGB->release();
        }
    } else {
        if (_imp->supportsGLSL)
            _imp->shaderBlack->release();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLErrors();
    if (_imp->overlay) {
        drawOverlay();
    }

    if (_imp->displayPersistentMessage) {
        drawPersistentMessage();
    }
    assert_checkGLErrors();
}


void ViewerGL::clearColorBuffer(double r ,double g ,double b ,double a )
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ViewerGL::toggleOverlays()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->overlay = !_imp->overlay;
    updateGL();
}


void ViewerGL::drawOverlay()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    const RectI& dispW = getDisplayWindow();
    
    
    renderText(dispW.right(),dispW.bottom(), _imp->resolutionOverlay,_imp->textRenderingColor,*_imp->textFont);
    
    QPoint topRight(dispW.right(),dispW.top());
    QPoint topLeft(dispW.left(),dispW.top());
    QPoint btmLeft(dispW.left(),dispW.bottom());
    QPoint btmRight(dispW.right(),dispW.bottom() );
    
    glBegin(GL_LINES);
    
    glColor4f( _imp->displayWindowOverlayColor.redF(),
              _imp->displayWindowOverlayColor.greenF(),
              _imp->displayWindowOverlayColor.blueF(),
              _imp->displayWindowOverlayColor.alphaF() );
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
    
    const RectI& dataW = getRoD();
    if(dispW != dataW){
        
        renderText(dataW.right(), dataW.top(), _imp->topRightBBOXoverlay, _imp->rodOverlayColor,*_imp->textFont);
        renderText(dataW.left(), dataW.bottom(), _imp->btmLeftBBOXoverlay, _imp->rodOverlayColor,*_imp->textFont);
        
        
        QPoint topRight2(dataW.right(), dataW.top());
        QPoint topLeft2(dataW.left(),dataW.top());
        QPoint btmLeft2(dataW.left(),dataW.bottom() );
        QPoint btmRight2(dataW.right(),dataW.bottom() );
        glPushAttrib(GL_LINE_BIT);
        glLineStipple(2, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);
        glBegin(GL_LINES);
        glColor4f( _imp->rodOverlayColor.redF(),
                  _imp->rodOverlayColor.greenF(),
                  _imp->rodOverlayColor.blueF(),
                  _imp->rodOverlayColor.alphaF() );
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
    
    if (_imp->isUserRoIEnabled) {
        drawUserRoI();
    }
    
    _imp->viewerTab->drawOverlays();

    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
    checkGLErrors();
}

void ViewerGL::drawUserRoI()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    glColor4f(0.9, 0.9, 0.9, 1.);
    
    
    ///base rect
    glBegin(GL_LINE_STRIP);
    glVertex2f(_imp->userRoI.x1, _imp->userRoI.y1); //bottom left
    glVertex2f(_imp->userRoI.x1, _imp->userRoI.y2); //top left
    glVertex2f(_imp->userRoI.x2, _imp->userRoI.y2); //top right
    glVertex2f(_imp->userRoI.x2, _imp->userRoI.y1); //bottom right
    glVertex2f(_imp->userRoI.x1, _imp->userRoI.y1); //bottom left
    glEnd();
    
    
    glBegin(GL_LINES);
    ///border ticks
    glVertex2f(_imp->userRoI.x1, (_imp->userRoI.y1 + _imp->userRoI.y2) / 2);
    glVertex2f(_imp->userRoI.x1 - USER_ROI_BORDER_TICK_SIZE / _imp->zoomCtx.zoomFactor, (_imp->userRoI.y1 + _imp->userRoI.y2) / 2);
    
    glVertex2f(_imp->userRoI.x2, (_imp->userRoI.y1 + _imp->userRoI.y2) / 2);
    glVertex2f(_imp->userRoI.x2 + USER_ROI_BORDER_TICK_SIZE / _imp->zoomCtx.zoomFactor, (_imp->userRoI.y1 + _imp->userRoI.y2) / 2);
    
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2, _imp->userRoI.y2);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2, _imp->userRoI.y2 + USER_ROI_BORDER_TICK_SIZE / _imp->zoomCtx.zoomFactor);
    
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2, _imp->userRoI.y1);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2, _imp->userRoI.y1 - USER_ROI_BORDER_TICK_SIZE / _imp->zoomCtx.zoomFactor);
    
    ///middle cross
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2  - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2);
    glEnd();
    
    int rectHalfSize = (USER_ROI_SELECTION_POINT_SIZE / (2.f * _imp->zoomCtx.zoomFactor));
    
    ///draw handles hint for the user
    glBegin(GL_QUADS);
    
    //left
    glVertex2f(_imp->userRoI.x1 + rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - rectHalfSize);
    glVertex2f(_imp->userRoI.x1 + rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x1 - rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x1 - rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - rectHalfSize);
    
    //top
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 - rectHalfSize,
               _imp->userRoI.y2 - rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 - rectHalfSize,
               _imp->userRoI.y2 + rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 + rectHalfSize,
               _imp->userRoI.y2 + rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 + rectHalfSize,
               _imp->userRoI.y2 - rectHalfSize);

    //right
    glVertex2f(_imp->userRoI.x2 - rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - rectHalfSize);
    glVertex2f(_imp->userRoI.x2 - rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x2 + rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x2 + rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - rectHalfSize);
    
    //bottom
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 - rectHalfSize,
               _imp->userRoI.y1 - rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 - rectHalfSize,
               _imp->userRoI.y1 + rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 + rectHalfSize,
               _imp->userRoI.y1 + rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 + rectHalfSize,
               _imp->userRoI.y1 - rectHalfSize);

    //middle
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 - rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 - rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 + rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + rectHalfSize);
    glVertex2f((_imp->userRoI.x1 +  _imp->userRoI.x2) / 2 + rectHalfSize,
               (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - rectHalfSize);
    
    
    //top left
    glVertex2f(_imp->userRoI.x1 - rectHalfSize, _imp->userRoI.y2 - rectHalfSize);
    glVertex2f(_imp->userRoI.x1 - rectHalfSize, _imp->userRoI.y2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x1 + rectHalfSize, _imp->userRoI.y2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x1 + rectHalfSize, _imp->userRoI.y2 - rectHalfSize);
    
    //top right
    glVertex2f(_imp->userRoI.x2 - rectHalfSize, _imp->userRoI.y2 - rectHalfSize);
    glVertex2f(_imp->userRoI.x2 - rectHalfSize, _imp->userRoI.y2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x2 + rectHalfSize, _imp->userRoI.y2 + rectHalfSize);
    glVertex2f(_imp->userRoI.x2 + rectHalfSize, _imp->userRoI.y2 - rectHalfSize);
    
    //bottom right
    glVertex2f(_imp->userRoI.x2 - rectHalfSize, _imp->userRoI.y1 - rectHalfSize);
    glVertex2f(_imp->userRoI.x2 - rectHalfSize, _imp->userRoI.y1 + rectHalfSize);
    glVertex2f(_imp->userRoI.x2 + rectHalfSize, _imp->userRoI.y1 + rectHalfSize);
    glVertex2f(_imp->userRoI.x2 + rectHalfSize, _imp->userRoI.y1 - rectHalfSize);

    
    //bottom left
    glVertex2f(_imp->userRoI.x1 - rectHalfSize, _imp->userRoI.y1 - rectHalfSize);
    glVertex2f(_imp->userRoI.x1 - rectHalfSize, _imp->userRoI.y1 + rectHalfSize);
    glVertex2f(_imp->userRoI.x1 + rectHalfSize, _imp->userRoI.y1 + rectHalfSize);
    glVertex2f(_imp->userRoI.x1 + rectHalfSize, _imp->userRoI.y1 - rectHalfSize);

    glEnd();
    
    
}


/**
 *@brief Resets the mouse position
 **/
void ViewerGL::resetMousePos()
{
    assert(!_imp->zoomCtx.zoomContextLock.tryLock());
    _imp->zoomCtx.oldClick.setX(0);
    _imp->zoomCtx.oldClick.setY(0);
}

void ViewerGL::drawPersistentMessage()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    QFontMetrics metrics(font());
    int numberOfLines = std::ceil((double)metrics.width(_imp->persistentMessage)/(double)(width()-20));
    int averageCharsPerLine = numberOfLines != 0 ? _imp->persistentMessage.size() / numberOfLines : _imp->persistentMessage.size();
    QStringList lines;
   
    int i = 0;
    while(i < _imp->persistentMessage.size()) {
        QString str;
        while(i < _imp->persistentMessage.size()){
            if(i%averageCharsPerLine == 0 && i!=0){
                break;
            }
            str.append(_imp->persistentMessage.at(i));
            ++i;
        }
        /*Find closest word end and insert a new line*/
        while(i < _imp->persistentMessage.size() && _imp->persistentMessage.at(i)!=QChar(' ')){
            str.append(_imp->persistentMessage.at(i));
            ++i;
        }
        lines.append(str);
    }
    
    
    QPointF topLeft = toImgCoordinates_fast(0,0);
    QPointF bottomRight = toImgCoordinates_fast(width(),numberOfLines*(metrics.height()*2));
    
    if(_imp->persistentMessageType == 1){ // error
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
    
    

    
    int offset = metrics.height()+10;
    for(int j = 0 ; j < lines.size();++j){
        QPointF pos = toImgCoordinates_fast(20, offset);
        renderText(pos.x(),pos.y(), lines.at(j),_imp->textRenderingColor,*_imp->textFont);
        offset += metrics.height()*2;
    }
    checkGLErrors();
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
}



void ViewerGL::initializeGL()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
	makeCurrent();
    initAndCheckGlExtensions();
    _imp->blackTex = new Texture;
    _imp->defaultDisplayTexture = new Texture;
    
    
    // glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &_imp->vboVerticesId);
    glGenBuffers(1, &_imp->vboTexturesId);
    glGenBuffers(1 , &_imp->iboTriangleStripId);
    
    glBindBuffer(GL_ARRAY_BUFFER, _imp->vboTexturesId);
    glBufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, _imp->vboVerticesId);
    glBufferData(GL_ARRAY_BUFFER, 32*sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _imp->iboTriangleStripId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 28*sizeof(GLubyte), triangleStrip, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    checkGLErrors();
    
    
    if(_imp->supportsGLSL){
        initShaderGLSL();
        checkGLErrors();
    }
    
    initBlackTex();
    checkGLErrors();
}

GLuint ViewerGL::getPboID(int index)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    if(index >= (int)_imp->pboIds.size()){
        GLuint handle;
        glGenBuffersARB(1,&handle);
        _imp->pboIds.push_back(handle);
        return handle;
    }else{
        return _imp->pboIds[index];
    }
}

/**
 *@returns Returns the current zoom factor that is applied to the display.
 **/
double ViewerGL::getZoomFactor() const
{
    QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
    return _imp->zoomCtx.zoomFactor;
}

RectI ViewerGL::getImageRectangleDisplayed(const RectI& imageRoD)
{
    QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
    RectI ret;
    QPointF topLeft = toImgCoordinates_fast(0, 0);
    ret.x1 = std::floor(topLeft.x());
    ret.y2 = std::ceil(topLeft.y());
    QPointF bottomRight = toImgCoordinates_fast(width()-1, height()-1);
    ret.x2 = std::ceil(bottomRight.x());
    ret.y1 = std::floor(bottomRight.y());
    if (!ret.intersect(imageRoD, &ret)) {
        ret.clear();
    }
    return ret;
}



int ViewerGL::isExtensionSupported(const char *extension)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
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


void ViewerGL::initAndCheckGlExtensions()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
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

        //Natron::errorDialog("Viewer error","The viewer is unabgile to work without a proper version of GLSL.");
        //cout << "Warning : GLSL not present on this hardware, no material acceleration possible." << endl;
        _imp->supportsGLSL = false;
    }
}


void ViewerGL::activateShaderRGB()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    // we assume that:
    // - 8-bits textures are stored non-linear and must be displayer as is
    // - floating-point textures are linear and must be decompressed according to the given lut

    assert(_imp->supportsGLSL );
    // don't even bind the shader on 8-bits gamma-compressed textures
    if (!(getBitDepth() != OpenGLViewerI::BYTE)) {
        return;
    }
    if (!_imp->shaderRGB->bind()) {
        cout << qPrintable(_imp->shaderRGB->log()) << endl;
    }
    
    _imp->shaderRGB->setUniformValue("Tex", 0);
    _imp->shaderRGB->setUniformValue("expMult",  (GLfloat)_imp->viewerTab->getInternalNode()->getExposure());
    _imp->shaderRGB->setUniformValue("lut", (GLint)_imp->viewerTab->getInternalNode()->getLutType());
    
    
}

void ViewerGL::initShaderGLSL()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    if(!_imp->shaderLoaded && _imp->supportsGLSL){

		_imp->shaderBlack=new QGLShaderProgram(context());
        if(!_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Vertex,vertRGB))
            cout << qPrintable(_imp->shaderBlack->log()) << endl;
        if(!_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Fragment,blackFrag))
            cout << qPrintable(_imp->shaderBlack->log()) << endl;
        if(!_imp->shaderBlack->link()){
            cout << qPrintable(_imp->shaderBlack->log()) << endl;
        }

        _imp->shaderRGB=new QGLShaderProgram(context());
        if(!_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Vertex,vertRGB))
            cout << qPrintable(_imp->shaderRGB->log()) << endl;
        if(!_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Fragment,fragRGB))
            cout << qPrintable(_imp->shaderRGB->log()) << endl;
        
        if(!_imp->shaderRGB->link()){
            cout << qPrintable(_imp->shaderRGB->log()) << endl;
        }
        _imp->shaderLoaded = true;
    }
    
}

void ViewerGL::saveGLState()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
}

void ViewerGL::restoreGLState()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

void ViewerGL::initBlackTex()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    //fitToFormat(getDisplayWindow());
    
    TextureRect texSize(0, 0, 2048, 1556,2048,1556,1);
    
    assert_checkGLErrors();
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, getPboID(0));
    checkGLErrors();
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texSize.x2 * texSize.y2 *sizeof(U32), NULL, GL_DYNAMIC_DRAW_ARB);
    checkGLErrors();
    U32* frameData = (U32*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    assert(frameData);
    for(int i = 0 ; i < texSize.x2 * texSize.y2 ; ++i) {
        frameData[i] = ViewerInstance::toBGRA(0, 0, 0, 255);
    }
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
    checkGLErrors();
    _imp->blackTex->fillOrAllocateTexture(texSize,Texture::BYTE);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    checkGLErrors();
    _imp->must_initBlackTex = false;
}



void ViewerGL::transferBufferFromRAMtoGPU(const unsigned char* ramBuffer, size_t bytesCount, const TextureRect& region,int pboIndex)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    QMutexLocker locker(&_imp->textureMutex);
	(void)glGetError();
    GLint currentBoundPBO = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
	GLenum err = glGetError();
	if(err != GL_NO_ERROR || currentBoundPBO != 0) {
		 qDebug() << "(ViewerGL::allocateAndMapPBO): Another PBO is currently mapped, glMap failed." << endl;
	}

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, getPboID(pboIndex));
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);
    GLvoid *ret = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    assert(ret);
    
    memcpy(ret, (void*)ramBuffer, bytesCount);

    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    checkGLErrors();
    
    OpenGLViewerI::BitDepth bd = getBitDepth();
    if(bd == OpenGLViewerI::BYTE){
        _imp->defaultDisplayTexture->fillOrAllocateTexture(region,Texture::BYTE);
    }else if(bd == OpenGLViewerI::FLOAT || bd == OpenGLViewerI::HALF_FLOAT){
        
        //do 32bit fp textures either way, don't bother with half float. We might support it further on.
        _imp->defaultDisplayTexture->fillOrAllocateTexture(region,Texture::FLOAT);
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
    checkGLErrors();
    _imp->displayingImage = true;
}

/**
 *@returns Returns true if the graphic card supports GLSL.
 **/
bool ViewerGL::supportsGLSL() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->supportsGLSL;
}

#if QT_VERSION < 0x050000
#define QMouseEventLocalPos(e) (e->posF())
#else
#define QMouseEventLocalPos(e) (e->localPos())
#endif

void ViewerGL::mousePressEvent(QMouseEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(event->button() == Qt::RightButton){
        _imp->menu->exec(mapToGlobal(event->pos()));
        return;
    }
    
    _imp->zoomCtx.oldClick = event->pos();
    _imp->lastMousePosition = event->pos();
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _imp->ms = DRAGGING_IMAGE;
    } else if(event->button() == Qt::LeftButton &&
              event->modifiers().testFlag(Qt::ControlModifier) &&
              _imp->displayingImage) {
        float r,g,b,a;
        QPointF imgPos = toImgCoordinates_fast(event->x(), event->y());
        bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
        bool picked = _imp->viewerTab->getInternalNode()->getColorAt(imgPos.x(), imgPos.y(), &r, &g, &b, &a, linear);
        if (picked) {
            QColor pickerColor;
            pickerColor.setRedF(r);
            pickerColor.setGreenF(g);
            pickerColor.setBlueF(b);
            pickerColor.setAlphaF(a);
            _imp->viewerTab->getGui()->setColorPickersColor(pickerColor);
        }
    } else if(event->button() == Qt::LeftButton && isNearByUserRoIBottomEdge(event->pos())) {
        _imp->ms = DRAGGING_ROI_BOTTOM_EDGE;
    } else if(event->button() == Qt::LeftButton && isNearByUserRoILeftEdge(event->pos())) {
        _imp->ms = DRAGGING_ROI_LEFT_EDGE;
    } else if(event->button() == Qt::LeftButton && isNearByUserRoIRightEdge(event->pos())) {
        _imp->ms = DRAGGING_ROI_RIGHT_EDGE;
    } else if(event->button() == Qt::LeftButton && isNearByUserRoITopEdge(event->pos())) {
        _imp->ms = DRAGGING_ROI_TOP_EDGE;
    } else if(event->button() == Qt::LeftButton && isNearByUserRoIMiddleHandle(event->pos())) {
        _imp->ms = DRAGGING_ROI_CROSS;
    } else if(event->button() == Qt::LeftButton && isNearByUserRoITopLeft(event->pos())) {
        _imp->ms = DRAGGING_ROI_TOP_LEFT;
    } else if(event->button() == Qt::LeftButton && isNearByUserRoITopRight(event->pos())) {
        _imp->ms = DRAGGING_ROI_TOP_RIGHT;
    }  else if(event->button() == Qt::LeftButton && isNearByUserRoIBottomLeft(event->pos())) {
        _imp->ms = DRAGGING_ROI_BOTTOM_LEFT;
    }  else if(event->button() == Qt::LeftButton && isNearByUserRoIBottomRight(event->pos())) {
        _imp->ms = DRAGGING_ROI_BOTTOM_RIGHT;
    }  else if (event->button() == Qt::LeftButton && !event->modifiers().testFlag(Qt::ControlModifier)) {
        if(_imp->viewerTab->notifyOverlaysPenDown(QMouseEventLocalPos(event),
                                                  toImgCoordinates_fast(event->x(), event->y()))){
            updateGL();
        }
    }
    
}

void ViewerGL::mouseReleaseEvent(QMouseEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->ms = UNDEFINED;
    if(_imp->viewerTab->notifyOverlaysPenUp(QMouseEventLocalPos(event),
                                            toImgCoordinates_fast(event->x(), event->y()))){
        updateGL();
    }

}
void ViewerGL::mouseMoveEvent(QMouseEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF pos = toImgCoordinates_fast(event->x(), event->y());
    const Format& dispW = getDisplayWindow();
    // if the mouse is inside the image, update the color picker
    if (pos.x() >= dispW.left() &&
            pos.x() <= dispW.width() &&
            pos.y() >= dispW.bottom() &&
            pos.y() <= dispW.height() &&
            event->x() >= 0 && event->x() < width() &&
            event->y() >= 0 && event->y() < height()) {
        if (!_imp->infoViewer->colorAndMouseVisible()) {
            _imp->infoViewer->showColorAndMouseInfo();
        }
        boost::shared_ptr<VideoEngine> videoEngine = _imp->viewerTab->getInternalNode()->getVideoEngine();
        if (!videoEngine->isWorking()) {
            updateColorPicker(event->x(),event->y());
        }
        _imp->infoViewer->setMousePos(QPoint((int)pos.x(),(int)pos.y()));
        emit infoMousePosChanged();
    } else {
        if(_imp->infoViewer->colorAndMouseVisible()){
            _imp->infoViewer->hideColorAndMouseInfo();
        }
    }
    
    //update the cursor if it is hovering an overlay and we're not dragging the image
    if (_imp->ms != DRAGGING_IMAGE && _imp->overlay && _imp->isUserRoIEnabled) {
        if (isNearByUserRoIBottomEdge(event->pos()) || isNearByUserRoITopEdge(event->pos())
            || _imp->ms == DRAGGING_ROI_BOTTOM_EDGE || _imp->ms == DRAGGING_ROI_TOP_EDGE) {
            setCursor(QCursor(Qt::SizeVerCursor));
        } else if(isNearByUserRoILeftEdge(event->pos()) || isNearByUserRoIRightEdge(event->pos())
                  || _imp->ms == DRAGGING_ROI_LEFT_EDGE || _imp->ms == DRAGGING_ROI_RIGHT_EDGE) {
            setCursor(QCursor(Qt::SizeHorCursor));
        } else if(isNearByUserRoIMiddleHandle(event->pos()) || _imp->ms == DRAGGING_ROI_CROSS) {
            setCursor(QCursor(Qt::SizeAllCursor));
        } else if(isNearByUserRoIBottomRight(event->pos()) || isNearByUserRoITopLeft(event->pos()) ||
                  _imp->ms == DRAGGING_ROI_BOTTOM_RIGHT) {
            setCursor(QCursor(Qt::SizeFDiagCursor));

        } else if(isNearByUserRoIBottomLeft(event->pos()) || isNearByUserRoITopRight(event->pos())
                  || _imp->ms == DRAGGING_ROI_BOTTOM_LEFT) {
            setCursor(QCursor(Qt::SizeBDiagCursor));

        }else {
            setCursor(QCursor(Qt::ArrowCursor));
        }
    } else {
        setCursor(QCursor(Qt::ArrowCursor));
    }
    
    QPoint newClick =  event->pos();
    QPointF newClick_opengl = toImgCoordinates_fast(newClick.x(),newClick.y());
    QPointF oldClick_opengl = toImgCoordinates_fast(_imp->zoomCtx.oldClick.x(),_imp->zoomCtx.oldClick.y());
    float dy = (oldClick_opengl.y() - newClick_opengl.y());
    
    QPointF oldPosition_opengl = toImgCoordinates_fast(_imp->lastMousePosition.x(), _imp->lastMousePosition.y());
    int dxSinceLastMove = (oldPosition_opengl.x() - newClick_opengl.x());
    int dySinceLastMove = (oldPosition_opengl.y() - newClick_opengl.y());

    switch (_imp->ms) {
        case DRAGGING_IMAGE: {
            _imp->zoomCtx.bottom += dy;
            _imp->zoomCtx.left += (oldClick_opengl.x() - newClick_opengl.x());
            _imp->zoomCtx.oldClick = newClick;
            if(_imp->displayingImage){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
            }
            //  else {
            updateGL();
            // }
            _imp->zoomOrPannedSinceLastFit = true;
            // no need to update the color picker or mouse posn: they should be unchanged
        } break;
        case DRAGGING_ROI_BOTTOM_EDGE: {
            if ((_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
                if(_imp->displayingImage){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
                }
                updateGL();
            }
        } break;
        case DRAGGING_ROI_LEFT_EDGE: {
            if ((_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
                if(_imp->displayingImage){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
                }
                updateGL();
            }
        } break;
        case DRAGGING_ROI_RIGHT_EDGE: {
            if ((_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
                if(_imp->displayingImage){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
                }
                updateGL();
            }
        } break;
        case DRAGGING_ROI_TOP_EDGE: {
            if ((_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
                if(_imp->displayingImage){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
                }
                updateGL();
            }
        } break;
        case DRAGGING_ROI_CROSS: {
            _imp->userRoI.move(-dxSinceLastMove,-dySinceLastMove);
            if(_imp->displayingImage){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
            }
            updateGL();
        } break;
        case DRAGGING_ROI_TOP_LEFT: {
            if ((_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
            }
            if ((_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
            }
            if(_imp->displayingImage){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
            }
            updateGL();
        } break;
        case DRAGGING_ROI_TOP_RIGHT: {
            if ((_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
            }
            if ((_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
            }
            if(_imp->displayingImage){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
            }
            updateGL();        } break;
        case DRAGGING_ROI_BOTTOM_RIGHT: {
            if ((_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
            }
            if ((_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
            }
            if(_imp->displayingImage){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
            }
            updateGL();        } break;
        case DRAGGING_ROI_BOTTOM_LEFT: {
            if ((_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
            }
            if ((_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
            }
            if(_imp->displayingImage){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
            }
            updateGL();
        } break;
        default: {
            if(_imp->viewerTab->notifyOverlaysPenMotion(QMouseEventLocalPos(event),pos)){
                updateGL();
            }
        }break;
    }
  
    _imp->lastMousePosition = newClick;
    //FIXME: This is bugged, somehow we can't set our custom picker cursor...
//    if(_imp->viewerTab->getGui()->_projectGui->hasPickers()){
//        setCursor(appPTR->getColorPickerCursor());
//    }else{
//        setCursor(QCursor(Qt::ArrowCursor));
//    }
}


void ViewerGL::updateColorPicker(int x,int y)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (!_imp->displayingImage) {
        _imp->infoViewer->hideColorAndMouseInfo();
        return;
    }
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
    float r,g,b,a;
    QPointF imgPos = toImgCoordinates_fast(pos.x(), pos.y());
    
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool picked = _imp->viewerTab->getInternalNode()->getColorAt(imgPos.x(), imgPos.y(), &r, &g, &b, &a, linear);
    if (!picked) {
        _imp->infoViewer->hideColorAndMouseInfo();
    } else {
        //   cout << "r: " << color.x() << " g: " << color.y() << " b: " << color.z() << endl;
        _imp->infoViewer->setColor(r,g,b,a);
        emit infoColorUnderMouseChanged();
    }
}

void ViewerGL::wheelEvent(QWheelEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (event->orientation() != Qt::Vertical) {
        return;
    }
    
    {
        QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
        const double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
        double newZoomFactor = _imp->zoomCtx.zoomFactor * scaleFactor;
        if (newZoomFactor <= 0.01) {
            newZoomFactor = 0.01;
        } else if (newZoomFactor > 1024.) {
            newZoomFactor = 1024.;
        }
        QPointF zoomCenter = toImgCoordinates_fast(event->x(), event->y());
        double zoomRatio =   _imp->zoomCtx.zoomFactor / newZoomFactor;
        _imp->zoomCtx.left = zoomCenter.x() - (zoomCenter.x() - _imp->zoomCtx.left)*zoomRatio;
        _imp->zoomCtx.bottom = zoomCenter.y() - (zoomCenter.y() - _imp->zoomCtx.bottom)*zoomRatio;
        
        _imp->zoomCtx.zoomFactor = newZoomFactor;
        
        assert(0 < _imp->zoomCtx.zoomFactor && _imp->zoomCtx.zoomFactor <= 1024);
        int zoomValue = (int)(100*_imp->zoomCtx.zoomFactor);
        if (zoomValue == 0) {
            zoomValue = 1; // sometimes, floor(100*0.01) makes 0
        }
        assert(zoomValue > 0);
        emit zoomChanged(zoomValue);
        
        _imp->zoomOrPannedSinceLastFit = true;
    }
    if (_imp->displayingImage) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
    }
    updateGL();
    
    
}

void ViewerGL::zoomSlot(int v)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    
    {
        QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
        assert(v > 0);
        double newZoomFactor = v/100.;
        if(newZoomFactor < 0.01) {
            newZoomFactor = 0.01;
        } else if (newZoomFactor > 1024.) {
            newZoomFactor = 1024.;
        }
        double zoomRatio =   _imp->zoomCtx.zoomFactor / newZoomFactor;
        double w = (double)width();
        double h = (double)height();
        double bottom = _imp->zoomCtx.bottom;
        double left = _imp->zoomCtx.left;
        double top =  bottom +  h / (double)_imp->zoomCtx.zoomFactor;
        double right = left +  w / (double)_imp->zoomCtx.zoomFactor;
        
        _imp->zoomCtx.left = (right + left)/2. - zoomRatio * (right - left)/2.;
        _imp->zoomCtx.bottom = (top + bottom)/2. - zoomRatio * (top - bottom)/2.;
        
        _imp->zoomCtx.zoomFactor = newZoomFactor;
        assert(0 < _imp->zoomCtx.zoomFactor && _imp->zoomCtx.zoomFactor <= 1024);
    }
    if(_imp->displayingImage){
        // appPTR->clearPlaybackCache();
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
    } else {
        updateGL();
    }
}

void ViewerGL::zoomSlot(QString str)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    str.remove(QChar('%'));
    int v = str.toInt();
    assert(v > 0);
    zoomSlot(v);
}

///this should be locked externally when not used on the main thread
QPointF ViewerGL::toImgCoordinates_fast(double x,double y)
{

    assert(qApp &&
           (qApp->thread() == QThread::currentThread() ||
            (qApp->thread() != QThread::currentThread() && !_imp->zoomCtx.zoomContextLock.tryLock())));
    
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / _imp->zoomCtx.zoomFactor * _imp->currentViewerInfos.getDisplayWindow().getPixelAspect();
    double right = left +  w / _imp->zoomCtx.zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPointF ViewerGL::toWidgetCoordinates(double x, double y)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / _imp->zoomCtx.zoomFactor * _imp->currentViewerInfos.getDisplayWindow().getPixelAspect();
    double right = left +  w / _imp->zoomCtx.zoomFactor;
    return QPoint(((x - left)/(right - left))*w,(y - top)/((bottom - top))*h);
}

void ViewerGL::fitImageToFormat(const Format& rod)
{

    double h = rod.height();
    double w = rod.width();
    assert(h > 0. && w > 0.);
    double zoomFactor = height()/h;
    zoomFactor = (zoomFactor > 0.06) ? (zoomFactor-0.05) : std::max(zoomFactor,0.01);
    assert(zoomFactor>=0.01 && zoomFactor <= 1024);
    
    QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
    if (_imp->zoomCtx.zoomFactor != zoomFactor) {
        _imp->zoomCtx.zoomFactor = zoomFactor;
        l.unlock();
        emit zoomChanged(zoomFactor * 100);
        l.relock();
    }
    resetMousePos();
    _imp->zoomCtx.left = w/2.f - (width()/(2.f*_imp->zoomCtx.zoomFactor));
    _imp->zoomCtx.bottom = h/2.f - (height()/(2.f*_imp->zoomCtx.zoomFactor)) * rod.getPixelAspect();
    _imp->zoomOrPannedSinceLastFit = false;
}


/**
 *@returns Returns a pointer to the current viewer infos.
 **/
const ImageInfo& ViewerGL::getCurrentViewerInfos() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->currentViewerInfos;
}

ViewerTab* ViewerGL::getViewerTab() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->viewerTab;
}

/**
 *@brief Turns on the overlays on the viewer.
 **/
void ViewerGL::turnOnOverlay()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->overlay=true;
}

/**
 *@brief Turns off the overlays on the viewer.
 **/
void ViewerGL::turnOffOverlay()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->overlay=false;
}

void ViewerGL::setInfoViewer(InfoViewerWidget* i )
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->infoViewer = i;
    QObject::connect(this,SIGNAL(infoMousePosChanged()), _imp->infoViewer, SLOT(updateCoordMouse()));
    QObject::connect(this,SIGNAL(infoColorUnderMouseChanged()),_imp->infoViewer,SLOT(updateColor()));
    QObject::connect(this,SIGNAL(infoResolutionChanged()),_imp->infoViewer,SLOT(changeResolution()));
    QObject::connect(this,SIGNAL(infoDataWindowChanged()),_imp->infoViewer,SLOT(changeDataWindow()));
    
    
}




void ViewerGL::disconnectViewer()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (displayingImage()) {
        setRegionOfDefinition(_imp->blankViewerInfos.getRoD());
        clearViewer();
    }
}



/*The dataWindow of the currentFrame(BBOX)*/
const RectI& ViewerGL::getRoD() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->currentViewerInfos.getRoD();
}

/*The displayWindow of the currentFrame(Resolution)*/
const Format& ViewerGL::getDisplayWindow() const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->currentViewerInfos.getDisplayWindow();
}


void ViewerGL::setRegionOfDefinition(const RectI& rod)
{
    QMutexLocker l(&_imp->currentViewerInfosLock);
    _imp->currentViewerInfos.setRoD(rod);
    emit infoDataWindowChanged();
    _imp->btmLeftBBOXoverlay.clear();
    _imp->btmLeftBBOXoverlay.append(QString::number(rod.left()));
    _imp->btmLeftBBOXoverlay.append(",");
    _imp->btmLeftBBOXoverlay.append(QString::number(rod.bottom()));
    _imp->topRightBBOXoverlay.clear();
    _imp->topRightBBOXoverlay.append(QString::number(rod.right()));
    _imp->topRightBBOXoverlay.append(",");
    _imp->topRightBBOXoverlay.append(QString::number(rod.top()));
    
    
    
}


void ViewerGL::onProjectFormatChanged(const Format& format)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->currentViewerInfos.setDisplayWindow(format);
    _imp->blankViewerInfos.setDisplayWindow(format);
    _imp->blankViewerInfos.setRoD(format);
    emit infoResolutionChanged();
    _imp->resolutionOverlay.clear();
    _imp->resolutionOverlay.append(QString::number(format.width()));
    _imp->resolutionOverlay.append("x");
    _imp->resolutionOverlay.append(QString::number(format.height()));
    
    if (!_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject()) {
        fitImageToFormat(format);
    }
    
    if(_imp->displayingImage) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
    }

    if (!_imp->isUserRoISet) {
        _imp->userRoI = format;
        _imp->isUserRoISet = true;
    }
}

void ViewerGL::setClipToDisplayWindow(bool b)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QMutexLocker l(&_imp->clipToDisplayWindowMutex);
    _imp->clipToDisplayWindow = b;
    ViewerInstance* viewer = _imp->viewerTab->getInternalNode();
    assert(viewer);
    if (viewer->getUiContext() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject()) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
    }
}

bool ViewerGL::isClippingImageToProjectWindow() const
{
    QMutexLocker l(&_imp->clipToDisplayWindowMutex);
    return _imp->clipToDisplayWindow;
}

/*display black in the viewer*/
void ViewerGL::clearViewer()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    setDisplayingImage(false);
    updateGL();
}

/*overload of QT enter/leave/resize events*/
void ViewerGL::focusInEvent(QFocusEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(_imp->viewerTab->notifyOverlaysFocusGained()){
        updateGL();
    }
    QGLWidget::focusInEvent(event);
}

void ViewerGL::focusOutEvent(QFocusEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(_imp->viewerTab->notifyOverlaysFocusLost()){
        updateGL();
    }
    QGLWidget::focusOutEvent(event);
}

void ViewerGL::enterEvent(QEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    updateColorPicker();
    setFocus();
    QGLWidget::enterEvent(event);
}

void ViewerGL::leaveEvent(QEvent *event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->infoViewer->hideColorAndMouseInfo();
    QGLWidget::leaveEvent(event);
}

void ViewerGL::resizeEvent(QResizeEvent* event)
{ // public to hack the protected field
    assert(qApp && qApp->thread() == QThread::currentThread());
    QGLWidget::resizeEvent(event);
}

void ViewerGL::keyPressEvent(QKeyEvent* event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (event->key() == Qt::Key_O) {
        toggleOverlays();
    }
    
    if(event->isAutoRepeat()){
        if(_imp->viewerTab->notifyOverlaysKeyRepeat(event)){
            updateGL();
        }
    }else{
        if(_imp->viewerTab->notifyOverlaysKeyDown(event)){
            updateGL();
        }
    }
    QGLWidget::keyPressEvent(event);
}


void ViewerGL::keyReleaseEvent(QKeyEvent* event)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(_imp->viewerTab->notifyOverlaysKeyUp(event)){
        updateGL();
    }
}


OpenGLViewerI::BitDepth ViewerGL::getBitDepth() const
{
    
    ///the bitdepth value is locked by the knob holding that value itself.
    OpenGLViewerI::BitDepth e = (OpenGLViewerI::BitDepth)appPTR->getCurrentSettings()->getViewersBitDepth();
    
    ///supportsGLSL is set on the main thread only once on startup, it doesn't need to be protected.
    if(!_imp->supportsGLSL){
        e = OpenGLViewerI::BYTE;
    }
    return e;
}



void ViewerGL::populateMenu()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->menu->clear();
    QAction* displayOverlaysAction = new QAction("Display overlays",this);
    displayOverlaysAction->setCheckable(true);
    displayOverlaysAction->setChecked(true);
    QObject::connect(displayOverlaysAction,SIGNAL(triggered()),this,SLOT(toggleOverlays()));
    _imp->menu->addAction(displayOverlaysAction);
}

void ViewerGL::renderText( int x, int y, const QString &string,const QColor& color,const QFont& font)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    if (string.isEmpty()) {
        return;
    }

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    QPointF pos = toWidgetCoordinates(x, y);
    _imp->textRenderer.renderText(pos.x(),h-pos.y(),string,color,font);
    checkGLErrors();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(_imp->zoomCtx.lastOrthoLeft,_imp->zoomCtx.lastOrthoRight,_imp->zoomCtx.lastOrthoBottom,_imp->zoomCtx.lastOrthoTop,-1,1);
    glMatrixMode(GL_MODELVIEW);
}

void ViewerGL::setPersistentMessage(int type,const QString& message)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->persistentMessageType = type;
    _imp->persistentMessage = message;
    _imp->displayPersistentMessage = true;
    updateGL();
}

void ViewerGL::clearPersistentMessage()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (!_imp->displayPersistentMessage) {
        return;
    }
    _imp->displayPersistentMessage = false;
    updateGL();
}

void ViewerGL::getProjection(double &left,double &bottom,double &zoomFactor) const
{
    QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
    left = _imp->zoomCtx.left;
    bottom = _imp->zoomCtx.bottom;
    zoomFactor = _imp->zoomCtx.zoomFactor;
}

void ViewerGL::setProjection(double left,double bottom,double zoomFactor)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QMutexLocker l(&_imp->zoomCtx.zoomContextLock);
    _imp->zoomCtx.left = left;
    _imp->zoomCtx.bottom = bottom;
    _imp->zoomCtx.zoomFactor = zoomFactor;
    _imp->zoomOrPannedSinceLastFit = true;
}

void ViewerGL::setUserRoIEnabled(bool b)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QMutexLocker(&_imp->userRoIMutex);
    _imp->isUserRoIEnabled = b;
    if (_imp->displayingImage) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false,false);
    }
    update();
}

bool ViewerGL::isNearByUserRoITopEdge(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    int length = std::min(_imp->userRoI.x2 - _imp->userRoI.x1 - 10,(int)(USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor) * 2);
    RectI r(_imp->userRoI.x1 + length / 2,
            _imp->userRoI.y2 - USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.x2 - length / 2,
            _imp->userRoI.y2 + USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoIRightEdge(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    int length = std::min(_imp->userRoI.y2 - _imp->userRoI.y1 - 10,(int)(USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor) * 2);

    RectI r(_imp->userRoI.x2 - USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y1 + length / 2,
            _imp->userRoI.x2 + USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y2 - length / 2);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoILeftEdge(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    int length = std::min(_imp->userRoI.y2 - _imp->userRoI.y1 - 10,(int)(USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor) * 2);

    RectI r(_imp->userRoI.x1 - USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y1 + length / 2,
            _imp->userRoI.x1 + USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y2 - length / 2);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoIBottomEdge(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    int length = std::min(_imp->userRoI.x2 - _imp->userRoI.x1 - 10,(int)(USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor) * 2);

    RectI r(_imp->userRoI.x1 + length / 2,
            _imp->userRoI.y1 - USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.x2 - length / 2,
            _imp->userRoI.y1 + USER_ROI_CLICK_TOLERANCE / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoIMiddleHandle(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    RectI r((_imp->userRoI.x1 + _imp->userRoI.x2) / 2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            (_imp->userRoI.x1 + _imp->userRoI.x2) / 2 + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            (_imp->userRoI.y1 + _imp->userRoI.y2) / 2 + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoITopLeft(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    RectI r(_imp->userRoI.x1 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.x1  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y2  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoITopRight(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    RectI r(_imp->userRoI.x2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.x2  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y2  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoIBottomRight(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    RectI r(_imp->userRoI.x2 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y1 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.x2  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y1  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isNearByUserRoIBottomLeft(const QPoint& mousePos)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF openglPos = toImgCoordinates_fast(mousePos.x(), mousePos.y());
    RectI r(_imp->userRoI.x1 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y1 - USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.x1  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor,
            _imp->userRoI.y1  + USER_ROI_CROSS_RADIUS / _imp->zoomCtx.zoomFactor);
    return r.contains(openglPos.x(),openglPos.y());
}

bool ViewerGL::isUserRegionOfInterestEnabled() const
{
    QMutexLocker(&_imp->userRoIMutex);
    return _imp->isUserRoIEnabled;
}

const RectI& ViewerGL::getUserRegionOfInterest() const
{
    QMutexLocker(&_imp->userRoIMutex);
    return _imp->userRoI;
}

void ViewerGL::setUserRoI(const RectI& r)
{
    QMutexLocker(&_imp->userRoIMutex);
    _imp->userRoI = r;
}

/**
* @brief Swap the OpenGL buffers.
**/
void ViewerGL::swapOpenGLBuffers()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    swapBuffers();
}

/**
 * @brief Repaint
**/
void ViewerGL::redraw()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    update();
}

/**
* @brief Returns the width and height of the viewport in window coordinates.
**/
void ViewerGL::getViewportSize(double &width, double &height) const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    width = this->width();
    height = this->height();
}

/**
* @brief Returns the pixel scale of the viewport.
**/
void ViewerGL::getPixelScale(double& xScale, double& yScale) const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    xScale = 1. / getZoomFactor();
    yScale = xScale;
}

/**
* @brief Returns the colour of the background (i.e: clear color) of the viewport.
**/
void ViewerGL::getBackgroundColour(double &r, double &g, double &b) const
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    r = _imp->clearColor.redF();
    g = _imp->clearColor.greenF();
    b = _imp->clearColor.blueF();
}

void ViewerGL::makeOpenGLcontextCurrent()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    makeCurrent();
}

void ViewerGL::onViewerNodeNameChanged(const QString& name)
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->viewerTab->getGui()->unregisterTab(_imp->viewerTab);
    TabWidget* parent = dynamic_cast<TabWidget*>(_imp->viewerTab->parentWidget());
    if ( parent ) {
        parent->setTabName(_imp->viewerTab, name);
    }
    _imp->viewerTab->getGui()->registerTab(_imp->viewerTab);
}

void ViewerGL::removeGUI()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->viewerTab->getGui()->removeViewerTab(_imp->viewerTab, true);
}

int ViewerGL::getCurrentView() const
{
    ///protected in viewerTab
    return _imp->viewerTab->getCurrentView();
}

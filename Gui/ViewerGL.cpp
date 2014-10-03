//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "ViewerGL.h"

#include <cassert>
#include <map>

#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QApplication>
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

#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/Image.h"
#include "Engine/ImageInfo.h"
#include "Engine/Lut.h"
#include "Engine/MemoryFile.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"

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
#include "Gui/ZoomContext.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/NodeGraph.h"
#include "Gui/CurveWidget.h"
#include "Gui/Histogram.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

#define USER_ROI_BORDER_TICK_SIZE 15.f
#define USER_ROI_CROSS_RADIUS 15.f
#define USER_ROI_SELECTION_POINT_SIZE 8.f
#define USER_ROI_CLICK_TOLERANCE 8.f

#define WIPE_MIX_HANDLE_LENGTH 50.
#define WIPE_ROTATE_HANDLE_LENGTH 100.
#define WIPE_ROTATE_OFFSET 30


#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

/*This class is the the core of the viewer : what displays images, overlays, etc...
   Everything related to OpenGL will (almost always) be in this class */

//using namespace Imf;
//using namespace Imath;
using namespace Natron;
using std::cout; using std::endl;

namespace {
/**
 *@enum MOUSE_STATE
 *@brief basic state switching for mouse events
 **/
enum MOUSE_STATE
{
    SELECTING = 0,
    DRAGGING_IMAGE,
    DRAGGING_ROI_LEFT_EDGE,
    DRAGGING_ROI_RIGHT_EDGE,
    DRAGGING_ROI_TOP_EDGE,
    DRAGGING_ROI_BOTTOM_EDGE,
    DRAGGING_ROI_TOP_LEFT,
    DRAGGING_ROI_TOP_RIGHT,
    DRAGGING_ROI_BOTTOM_RIGHT,
    DRAGGING_ROI_BOTTOM_LEFT,
    DRAGGING_ROI_CROSS,
    PICKING_COLOR,
    BUILDING_PICKER_RECTANGLE,
    DRAGGING_WIPE_CENTER,
    DRAGGING_WIPE_MIX_HANDLE,
    ROTATING_WIPE_HANDLE,
    UNDEFINED
};

enum HOVER_STATE
{
    HOVERING_NOTHING = 0,
    HOVERING_WIPE_MIX,
    HOVERING_WIPE_ROTATE_HANDLE
};
} // namespace

enum PickerState
{
    PICKER_INACTIVE = 0,
    PICKER_POINT,
    PICKER_RECTANGLE
};

struct ViewerGL::Implementation
{
    Implementation(ViewerTab* parent,
                   ViewerGL* _this)
        : pboIds()
          , vboVerticesId(0)
          , vboTexturesId(0)
          , iboTriangleStripId(0)
          , activeTextures()
          , displayTextures()
          , shaderRGB(0)
          , shaderBlack(0)
          , shaderLoaded(false)
          , infoViewer()
          , viewerTab(parent)
          , zoomOrPannedSinceLastFit(false)
          , oldClick()
          , blankViewerInfos()
          , displayingImageGain()
          , displayingImageOffset()
          , displayingImageMipMapLevel(0)
          , displayingImagePremult()
          , displayingImageLut(Natron::sRGB)
          , ms(UNDEFINED)
          , hs(HOVERING_NOTHING)
          , textRenderingColor(200,200,200,255)
          , displayWindowOverlayColor(125,125,125,255)
          , rodOverlayColor(100,100,100,255)
          , textFont( new QFont(NATRON_FONT, NATRON_FONT_SIZE_13) )
          , overlay(true)
          , supportsGLSL(true)
          , updatingTexture(false)
          , clearColor(0,0,0,255)
          , menu( new QMenu(_this) )
          , persistentMessage()
          , persistentMessageType(0)
          , displayPersistentMessage(false)
          , textRenderer()
          , isUserRoISet(false)
          , lastMousePosition()
          , lastDragStartPos()
          , currentViewerInfos()
          , currentViewerInfos_btmLeftBBOXoverlay()
          , currentViewerInfos_topRightBBOXoverlay()
          , currentViewerInfos_resolutionOverlay()
          , pickerState(PICKER_INACTIVE)
          , lastPickerPos()
          , userRoIEnabled(false) // protected by mutex
          , userRoI() // protected by mutex
          , zoomCtx() // protected by mutex
          , clipToDisplayWindow(true) // protected by mutex
          , wipeControlsMutex()
          , mixAmount(1.) // protected by mutex
          , wipeAngle(M_PI / 2.) // protected by mutex
          , wipeCenter()
          , selectionRectangle()
          , checkerboardTextureID(0)
          , checkerboardTileSize(0)
          , savedTexture(0)
          , lastRenderedImageMutex()
          , lastRenderedImage()
          , memoryHeldByLastRenderedImages()
    {
        infoViewer[0] = 0;
        infoViewer[1] = 0;
        displayTextures[0] = 0;
        displayTextures[1] = 0;
        activeTextures[0] = 0;
        activeTextures[1] = 0;
        memoryHeldByLastRenderedImages[0] = memoryHeldByLastRenderedImages[1] = 0;
        displayingImageGain[0] = displayingImageGain[1] = 1.;
        displayingImageOffset[0] = displayingImageOffset[1] = 0.;
        assert( qApp && qApp->thread() == QThread::currentThread() );
        menu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );

    }

    /////////////////////////////////////////////////////////
    // The following are only accessed from the main thread:

    std::vector<GLuint> pboIds; //!< PBO's id's used by the OpenGL context
    //   GLuint vaoId; //!< VAO holding the rendering VBOs for texture mapping.
    GLuint vboVerticesId; //!< VBO holding the vertices for the texture mapping.
    GLuint vboTexturesId; //!< VBO holding texture coordinates.
    GLuint iboTriangleStripId; /*!< IBOs holding vertices indexes for triangle strip sets*/
    Texture* activeTextures[2]; /*!< A pointer to the current textures used to display. One for A and B. May point to blackTex */
    Texture* displayTextures[2]; /*!< A pointer to the textures that would be used if A and B are displayed*/
    QGLShaderProgram* shaderRGB; /*!< The shader program used to render RGB data*/
    QGLShaderProgram* shaderBlack; /*!< The shader program used when the viewer is disconnected.*/
    bool shaderLoaded; /*!< Flag to check whether the shaders have already been loaded.*/
    InfoViewerWidget* infoViewer[2]; /*!< Pointer to the info bar below the viewer holding pixel/mouse/format related infos*/
    ViewerTab* const viewerTab; /*!< Pointer to the viewer tab GUI*/
    bool zoomOrPannedSinceLastFit; //< true if the user zoomed or panned the image since the last call to fitToRoD
    QPoint oldClick;
    ImageInfo blankViewerInfos; /*!< Pointer to the infos used when the viewer is disconnected.*/
    double displayingImageGain[2];
    double displayingImageOffset[2];
    unsigned int displayingImageMipMapLevel;
    Natron::ImagePremultiplication displayingImagePremult[2];
    Natron::ViewerColorSpace displayingImageLut;
    MOUSE_STATE ms; /*!< Holds the mouse state*/
    HOVER_STATE hs;
    const QColor textRenderingColor;
    const QColor displayWindowOverlayColor;
    const QColor rodOverlayColor;
    QFont* textFont;
    bool overlay; /*!< True if the user enabled overlay dispay*/

    // supportsGLSL is accessed from several threads, but is set only once at startup
    bool supportsGLSL; /*!< True if the user has a GLSL version supporting everything requested.*/
    bool updatingTexture;
    QColor clearColor;
    QMenu* menu;
    QString persistentMessage;
    int persistentMessageType;
    bool displayPersistentMessage;
    Natron::TextRenderer textRenderer;
    bool isUserRoISet;
    QPoint lastMousePosition; //< in widget coordinates
    QPointF lastDragStartPos; //< in zoom coordinates

    /////// currentViewerInfos
    ImageInfo currentViewerInfos[2]; /*!< Pointer to the ViewerInfos  used for rendering*/
    QString currentViewerInfos_btmLeftBBOXoverlay[2]; /*!< The string holding the bottom left corner coordinates of the dataWindow*/
    QString currentViewerInfos_topRightBBOXoverlay[2]; /*!< The string holding the top right corner coordinates of the dataWindow*/
    QString currentViewerInfos_resolutionOverlay; /*!< The string holding the resolution overlay, e.g: "1920x1080"*/

    ///////Picker infos, used only by the main-thread
    PickerState pickerState;
    QPointF lastPickerPos;
    QRectF pickerRect;

    //////////////////////////////////////////////////////////
    // The following are accessed from various threads
    QMutex userRoIMutex;
    bool userRoIEnabled;
    RectD userRoI; //< in canonical coords
    ZoomContext zoomCtx; /*!< All zoom related variables are packed into this object. */
    mutable QMutex zoomCtxMutex; /// protectx zoomCtx*
    QMutex clipToDisplayWindowMutex;
    bool clipToDisplayWindow;
    mutable QMutex wipeControlsMutex;
    double mixAmount; /// the amount of the second input to blend, by default 1.
    double wipeAngle; /// the angle to the X axis
    QPointF wipeCenter; /// the center of the wipe control
    QRectF selectionRectangle;
    
    GLuint checkerboardTextureID;
    int checkerboardTileSize; // to avoid a call to getValue() of the settings at each draw

    GLuint savedTexture; // @see saveOpenGLContext/restoreOpenGLContext
    GLuint prevBoundTexture; // @see bindTextureAndActivateShader/unbindTextureAndReleaseShader

    mutable QMutex lastRenderedImageMutex; //protects lastRenderedImage & memoryHeldByLastRenderedImages
    boost::shared_ptr<Natron::Image> lastRenderedImage[2]; //<  last image passed to transferRAMBuffer
    U64 memoryHeldByLastRenderedImages[2];
    
    bool isNearbyWipeCenter(const QPointF & pos,double tolerance) const;
    bool isNearbyWipeRotateBar(const QPointF & pos,double tolerance) const;
    bool isNearbyWipeMixHandle(const QPointF & pos,double tolerance) const;

    void drawArcOfCircle(const QPointF & center,double radius,double startAngle,double endAngle);

    void bindTextureAndActivateShader(int i,
                                      bool useShader)
    {
        assert(activeTextures[i]);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&prevBoundTexture);
        glBindTexture( GL_TEXTURE_2D, activeTextures[i]->getTexID() );
        // debug (so the OpenGL debugger can make a breakpoint here)
        //GLfloat d;
        //glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &d);
        if (useShader) {
            activateShaderRGB(i);
        }
        glCheckError();
    }

    void unbindTextureAndReleaseShader(bool useShader)
    {
        if (useShader) {
            shaderRGB->release();
        }
        glCheckError();
        glBindTexture(GL_TEXTURE_2D, prevBoundTexture);
    }

    /**
     *@brief Starts using the RGB shader to display the frame
     **/
    void activateShaderRGB(int texIndex);

    enum WipePolygonType
    {
        POLYGON_EMPTY = 0,  // don't draw anything
        POLYGON_FULL,  // draw the whole texture as usual
        POLYGON_PARTIAL, // use the polygon returned to draw
    };

    WipePolygonType getWipePolygon(const RectD & texRectClipped, //!< in canonical coordinates
                                   QPolygonF & polygonPoints,
                                   bool rightPlane) const;

    static void getBaseTextureCoordinates(const RectI & texRect,int closestPo2,int texW,int texH,
                                          GLfloat & bottom,GLfloat & top,GLfloat & left,GLfloat & right);
    static void getPolygonTextureCoordinates(const QPolygonF & polygonPoints,
                                             const RectD & texRect, //!< in canonical coordinates
                                             QPolygonF & texCoords);

    void refreshSelectionRectangle(const QPointF & pos);

    void drawSelectionRectangle();
    
    void initializeCheckerboardTexture(bool mustCreateTexture);
    
    void drawCheckerboardTexture(const RectD& rod);
};

#if 0
/**
 *@brief Actually converting to ARGB... but it is called BGRA by
   the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

#endif
//static const GLfloat renderingTextureCoordinates[32] = {
//    0 , 1 , //0
//    0 , 1 , //1
//    1 , 1 , //2
//    1 , 1 , //3
//    0 , 1 , //4
//    0 , 1 , //5
//    1 , 1 , //6
//    1 , 1 , //7
//    0 , 0 , //8
//    0 , 0 , //9
//    1 , 0 , //10
//    1 , 0 , //11
//    0 , 0 , //12
//    0 , 0 , //13
//    1 , 0 , //14
//    1 , 0   //15
//};

/*see http://www.learnopengles.com/android-lesson-eight-an-introduction-to-index-buffer-objects-ibos/ */
static const GLubyte triangleStrip[28] = {
    0,4,1,5,2,6,3,7,
    7,4,
    4,8,5,9,6,10,7,11,
    11,8,
    8,12,9,13,10,14,11,15
};

/*
   ASCII art of the vertices used to render.
   The actual texture seen on the viewport is the rect (5,9,10,6).
   We draw  3*6 strips

 0___1___2___3
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
static GLfloat
clipTexCoord(const double clippedSize,
             const double size,
             const double texCoordRange)
{
    return clippedSize / size * texCoordRange;
}

/**
 * @brief Clips texBottom,texTop,texLeft,texRight (which are tex coordinates expressed in normalized
 * coordinates of "rect") against "clippedRect" which is a clipped portion of rect.
 **/
static void
clipTexCoords(const RectD & rect,
              const RectD & clippedRect,
              GLfloat & texBottom,
              GLfloat & texTop,
              GLfloat & texLeft,
              GLfloat & texRight)
{
    const double texHeight = texTop - texBottom;
    const double texWidth = texRight - texLeft;

    texBottom = clipTexCoord(clippedRect.y1 - rect.y1,rect.height(),texHeight);
    texTop = clipTexCoord(clippedRect.y2 - rect.y1,rect.height(),texHeight);
    texLeft = clipTexCoord(clippedRect.x1 - rect.x1,rect.width(),texWidth);
    texRight = clipTexCoord(clippedRect.x2 - rect.x1,rect.width(),texWidth);
}

void
ViewerGL::drawRenderingVAO(unsigned int mipMapLevel,
                           int textureIndex,
                           ViewerGL::DrawPolygonMode polygonMode)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    bool useShader = getBitDepth() != OpenGLViewerI::BYTE && _imp->supportsGLSL;

    
    ///the texture rectangle in image coordinates. The values in it are multiples of tile size.
    ///
    const TextureRect &r = _imp->activeTextures[textureIndex]->getTextureRect();

    ///This is the coordinates in the image being rendered where datas are valid, this is in pixel coordinates
    ///at the time we initialize it but we will convert it later to canonical coordinates. See 1)
    RectI texRect(r.x1,r.y1,r.x2,r.y2);


    ///the RoD of the image in canonical coords.
    RectD rod = getRoD(textureIndex);
    bool clipToDisplayWindow;
    {
        QMutexLocker l(&_imp->clipToDisplayWindowMutex);
        clipToDisplayWindow = _imp->clipToDisplayWindow;
    }
    if (clipToDisplayWindow) {
        ///clip the RoD to the project format.
        if ( !rod.intersect(getDisplayWindow(),&rod) ) {
            return;
        }
    }
   

    RectD canonicalTexRect;
    texRect.toCanonical(mipMapLevel, rod, &canonicalTexRect);

    //if user RoI is enabled, clip the rod to that roi
    bool userRoiEnabled;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoiEnabled = _imp->userRoIEnabled;
    }


    ////The texture real size (r.w,r.h) might be slightly bigger than the actual
    ////pixel coordinates bounds r.x1,r.x2 r.y1 r.y2 because we clipped these bounds against the bounds
    ////in the ViewerInstance::renderViewer function. That means we need to draw actually only the part of
    ////the texture that contains the bounds.
    ////Notice that r.w and r.h are scaled to the closest Po2 of the current scaling factor, so we need to scale it up
    ////So it is in the same coordinates as the bounds.
    GLfloat texBottom =  0;
    GLfloat texTop =  (GLfloat)(r.y2 - r.y1)  / (GLfloat)(r.h /** r.closestPo2*/);
    GLfloat texLeft = 0;
    GLfloat texRight = (GLfloat)(r.x2 - r.x1)  / (GLfloat)(r.w /** r.closestPo2*/);
    RectD rectClippedToRoI(canonicalTexRect);
    if (userRoiEnabled) {
        {
            QMutexLocker l(&_imp->userRoIMutex);
            //if the userRoI isn't intersecting the rod, just don't render anything
            if ( !rod.intersect(_imp->userRoI,&rod) ) {
                return;
            }
        }
        canonicalTexRect.intersect(rod, &rectClippedToRoI);
        clipTexCoords(canonicalTexRect,rectClippedToRoI,texBottom,texTop,texLeft,texRight);
    }

    if (polygonMode != ALL_PLANE) {
        /// draw only  the plane defined by the wipe handle
        QPolygonF polygonPoints,polygonTexCoords;
        Implementation::WipePolygonType polyType = _imp->getWipePolygon(rectClippedToRoI, polygonPoints, polygonMode == WIPE_RIGHT_PLANE);

        if (polyType == Implementation::POLYGON_EMPTY) {
            ///don't draw anything
            return;
        } else if (polyType == Implementation::POLYGON_PARTIAL) {
            _imp->getPolygonTextureCoordinates(polygonPoints, canonicalTexRect, polygonTexCoords);

            _imp->bindTextureAndActivateShader(textureIndex, useShader);

            glBegin(GL_POLYGON);
            for (int i = 0; i < polygonTexCoords.size(); ++i) {
                const QPointF & tCoord = polygonTexCoords[i];
                const QPointF & vCoord = polygonPoints[i];
                glTexCoord2d( tCoord.x(), tCoord.y() );
                glVertex2d( vCoord.x(), vCoord.y() );
            }
            glEnd();
            
            _imp->unbindTextureAndReleaseShader(useShader);

        } else {
            ///draw the all polygon as usual
            polygonMode = ALL_PLANE;
        }
    }

    if (polygonMode == ALL_PLANE) {
        ///Vertices are in canonical coords
        GLfloat vertices[32] = {
            (GLfloat)rod.left(),(GLfloat)rod.top(),    //0
            (GLfloat)rectClippedToRoI.x1, (GLfloat)rod.top(),          //1
            (GLfloat)rectClippedToRoI.x2, (GLfloat)rod.top(),    //2
            (GLfloat)rod.right(),(GLfloat)rod.top(),   //3
            (GLfloat)rod.left(), (GLfloat)rectClippedToRoI.y2, //4
            (GLfloat)rectClippedToRoI.x1,  (GLfloat)rectClippedToRoI.y2,       //5
            (GLfloat)rectClippedToRoI.x2,  (GLfloat)rectClippedToRoI.y2, //6
            (GLfloat)rod.right(),(GLfloat)rectClippedToRoI.y2, //7
            (GLfloat)rod.left(),(GLfloat)rectClippedToRoI.y1,        //8
            (GLfloat)rectClippedToRoI.x1,  (GLfloat)rectClippedToRoI.y1,             //9
            (GLfloat)rectClippedToRoI.x2,  (GLfloat)rectClippedToRoI.y1,       //10
            (GLfloat)rod.right(),(GLfloat)rectClippedToRoI.y1,       //11
            (GLfloat)rod.left(), (GLfloat)rod.bottom(), //12
            (GLfloat)rectClippedToRoI.x1,  (GLfloat)rod.bottom(),       //13
            (GLfloat)rectClippedToRoI.x2,  (GLfloat)rod.bottom(), //14
            (GLfloat)rod.right(),(GLfloat)rod.bottom() //15
        };
        GLfloat renderingTextureCoordinates[32] = {
            texLeft, texTop,   //0
            texLeft, texTop,   //1
            texRight, texTop,  //2
            texRight, texTop,   //3
            texLeft, texTop,   //4
            texLeft, texTop,   //5
            texRight, texTop,   //6
            texRight, texTop,   //7
            texLeft, texBottom,   //8
            texLeft, texBottom,   //9
            texRight, texBottom,    //10
            texRight, texBottom,   //11
            texLeft, texBottom,   // 12
            texLeft, texBottom,   //13
            texRight, texBottom,   //14
            texRight, texBottom    //15
        };

        
        if (_imp->viewerTab->isCheckerboardEnabled()) {
            _imp->drawCheckerboardTexture(rod);
        }
        
        _imp->bindTextureAndActivateShader(textureIndex, useShader);

        glCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, _imp->vboVerticesId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 32 * sizeof(GLfloat), vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, _imp->vboTexturesId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 32 * sizeof(GLfloat), renderingTextureCoordinates);
        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER,0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _imp->iboTriangleStripId);
        glDrawElements(GL_TRIANGLE_STRIP, 28, GL_UNSIGNED_BYTE, 0);
        glCheckErrorIgnoreOSXBug();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glCheckError();
        
        _imp->unbindTextureAndReleaseShader(useShader);
    }
} // drawRenderingVAO

void
ViewerGL::Implementation::getBaseTextureCoordinates(const RectI & r,
                                                    int closestPo2,
                                                    int texW,
                                                    int texH,
                                                    GLfloat & bottom,
                                                    GLfloat & top,
                                                    GLfloat & left,
                                                    GLfloat & right)
{
    bottom =  0;
    top =  (GLfloat)(r.y2 - r.y1)  / (GLfloat)(texH * closestPo2);
    left = 0;
    right = (GLfloat)(r.x2 - r.x1)  / (GLfloat)(texW * closestPo2);
}

void
ViewerGL::Implementation::getPolygonTextureCoordinates(const QPolygonF & polygonPoints,
                                                       const RectD & texRect,
                                                       QPolygonF & texCoords)
{
    texCoords.resize( polygonPoints.size() );
    for (int i = 0; i < polygonPoints.size(); ++i) {
        const QPointF & polygonPoint = polygonPoints.at(i);
        QPointF texCoord;
        texCoord.setX( (polygonPoint.x() - texRect.x1) / texRect.width() ); // * (right - left));
        texCoord.setY( (polygonPoint.y() - texRect.y1) / texRect.height() ); // * (top - bottom));
        texCoords[i] = texCoord;
    }
}

ViewerGL::Implementation::WipePolygonType
ViewerGL::Implementation::getWipePolygon(const RectD & texRectClipped,
                                         QPolygonF & polygonPoints,
                                         bool rightPlane) const
{
    ///Compute a second point on the plane separator line
    ///we don't really care how far it is from the center point, it just has to be on the line
    QPointF firstPoint,secondPoint;
    double mpi2 = M_PI / 2.;
    QPointF center;
    double angle;
    {
        QMutexLocker l(&wipeControlsMutex);
        center = wipeCenter;
        angle = wipeAngle;
    }

    ///extrapolate the line to the maximum size of the RoD so we're sure the line
    ///intersection algorithm works
    double maxSize = std::max(texRectClipped.x2 - texRectClipped.x1,texRectClipped.y2 - texRectClipped.y1) * 10000.;
    double xmax,ymax;

    xmax = std::cos(angle + mpi2) * maxSize;
    ymax = std::sin(angle + mpi2) * maxSize;

    firstPoint.setX(center.x() - xmax);
    firstPoint.setY(center.y() - ymax);
    secondPoint.setX(center.x() + xmax);
    secondPoint.setY(center.y() + ymax);

    QLineF inter(firstPoint,secondPoint);
    QLineF::IntersectType intersectionTypes[4];
    QPointF intersections[4];
    QLineF topEdge(texRectClipped.x1,texRectClipped.y2,texRectClipped.x2,texRectClipped.y2);
    QLineF rightEdge(texRectClipped.x2,texRectClipped.y2,texRectClipped.x2,texRectClipped.y1);
    QLineF bottomEdge(texRectClipped.x2,texRectClipped.y1,texRectClipped.x1,texRectClipped.y1);
    QLineF leftEdge(texRectClipped.x1,texRectClipped.y1,texRectClipped.x1,texRectClipped.y2);
    bool crossingTop = false,crossingRight = false,crossingLeft = false,crossingBtm = false;
    int validIntersectionsIndex[2];
    validIntersectionsIndex[0] = validIntersectionsIndex[1] = -1;
    int numIntersec = 0;
    intersectionTypes[0] = inter.intersect(topEdge, &intersections[0]);
    if (intersectionTypes[0] == QLineF::BoundedIntersection) {
        validIntersectionsIndex[numIntersec] = 0;
        crossingTop = true;
        ++numIntersec;
    }
    intersectionTypes[1] = inter.intersect(rightEdge, &intersections[1]);
    if (intersectionTypes[1]  == QLineF::BoundedIntersection) {
        validIntersectionsIndex[numIntersec] = 1;
        crossingRight = true;
        ++numIntersec;
    }
    intersectionTypes[2] = inter.intersect(bottomEdge, &intersections[2]);
    if (intersectionTypes[2]  == QLineF::BoundedIntersection) {
        validIntersectionsIndex[numIntersec] = 2;
        crossingBtm = true;
        ++numIntersec;
    }
    intersectionTypes[3] = inter.intersect(leftEdge, &intersections[3]);
    if (intersectionTypes[3]  == QLineF::BoundedIntersection) {
        validIntersectionsIndex[numIntersec] = 3;
        crossingLeft = true;
        ++numIntersec;
    }

    if ( (numIntersec != 0) && (numIntersec != 2) ) {
        ///Don't bother drawing the polygon, it is most certainly not visible in this case
        return ViewerGL::Implementation::POLYGON_EMPTY;
    }

    ///determine the orientation of the planes
    double crossProd  = ( secondPoint.x() - center.x() ) * ( texRectClipped.y1 - center.y() )
                        - ( secondPoint.y() - center.y() ) * ( texRectClipped.x1 - center.x() );
    if (numIntersec == 0) {
        ///the bottom left corner is on the left plane
        if ( (crossProd > 0) && ( (center.x() >= texRectClipped.x2) || (center.y() >= texRectClipped.y2) ) ) {
            ///the plane is invisible because the wipe handle is below or on the left of the texRectClipped
            return rightPlane ? ViewerGL::Implementation::POLYGON_EMPTY : ViewerGL::Implementation::POLYGON_FULL;
        }

        ///otherwise we draw the entire texture as usual
        return rightPlane ? ViewerGL::Implementation::POLYGON_FULL : ViewerGL::Implementation::POLYGON_EMPTY;
    } else {
        ///we have 2 intersects
        assert(validIntersectionsIndex[0] != -1 && validIntersectionsIndex[1] != -1);
        bool isBottomLeftOnLeftPlane = crossProd > 0;

        ///there are 6 cases
        if (crossingBtm && crossingLeft) {
            if ( (isBottomLeftOnLeftPlane && rightPlane) || (!isBottomLeftOnLeftPlane && !rightPlane) ) {
                //btm intersect is the first
                polygonPoints.insert(0,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x1,texRectClipped.y2) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x2,texRectClipped.y2) );
                polygonPoints.insert( 4,QPointF(texRectClipped.x2,texRectClipped.y1) );
                polygonPoints.insert(5,intersections[validIntersectionsIndex[0]]);
            } else {
                polygonPoints.insert(0,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x1,texRectClipped.y1) );
                polygonPoints.insert(3,intersections[validIntersectionsIndex[0]]);
            }
        } else if (crossingBtm && crossingTop) {
            if ( (isBottomLeftOnLeftPlane && rightPlane) || (!isBottomLeftOnLeftPlane && !rightPlane) ) {
                ///btm intersect is the second
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y2) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x2,texRectClipped.y1) );
                polygonPoints.insert(4,intersections[validIntersectionsIndex[1]]);
            } else {
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x1,texRectClipped.y2) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x1,texRectClipped.y1) );
                polygonPoints.insert(4,intersections[validIntersectionsIndex[1]]);
            }
        } else if (crossingBtm && crossingRight) {
            if ( (isBottomLeftOnLeftPlane && rightPlane) || (!isBottomLeftOnLeftPlane && !rightPlane) ) {
                ///btm intersect is the second
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y1) );
                polygonPoints.insert(3,intersections[validIntersectionsIndex[1]]);
            } else {
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y2) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x1,texRectClipped.y2) );
                polygonPoints.insert( 4,QPointF(texRectClipped.x1,texRectClipped.y1) );
                polygonPoints.insert(5,intersections[validIntersectionsIndex[1]]);
            }
        } else if (crossingLeft && crossingTop) {
            if ( (isBottomLeftOnLeftPlane && rightPlane) || (!isBottomLeftOnLeftPlane && !rightPlane) ) {
                ///left intersect is the second
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x1,texRectClipped.y2) );
                polygonPoints.insert(3,intersections[validIntersectionsIndex[1]]);
            } else {
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y2) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x2,texRectClipped.y1) );
                polygonPoints.insert( 4,QPointF(texRectClipped.x1,texRectClipped.y1) );
                polygonPoints.insert(5,intersections[validIntersectionsIndex[1]]);
            }
        } else if (crossingLeft && crossingRight) {
            if ( (isBottomLeftOnLeftPlane && rightPlane) || (!isBottomLeftOnLeftPlane && !rightPlane) ) {
                ///left intersect is the second
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert( 1,QPointF(texRectClipped.x1,texRectClipped.y2) );
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y2) );
                polygonPoints.insert(3,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert(4,intersections[validIntersectionsIndex[1]]);
            } else {
                polygonPoints.insert(0,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y1) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x1,texRectClipped.y1) );
                polygonPoints.insert(4,intersections[validIntersectionsIndex[1]]);
            }
        } else if (crossingTop && crossingRight) {
            if ( (isBottomLeftOnLeftPlane && rightPlane) || (!isBottomLeftOnLeftPlane && !rightPlane) ) {
                ///right is second
                polygonPoints.insert(0,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert( 1,QPointF(texRectClipped.x2,texRectClipped.y2) );
                polygonPoints.insert(2,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert(3,intersections[validIntersectionsIndex[0]]);
            } else {
                polygonPoints.insert(0,intersections[validIntersectionsIndex[0]]);
                polygonPoints.insert(1,intersections[validIntersectionsIndex[1]]);
                polygonPoints.insert( 2,QPointF(texRectClipped.x2,texRectClipped.y1) );
                polygonPoints.insert( 3,QPointF(texRectClipped.x1,texRectClipped.y1) );
                polygonPoints.insert( 4,QPointF(texRectClipped.x1,texRectClipped.y2) );
                polygonPoints.insert(5,intersections[validIntersectionsIndex[0]]);
            }
        } else {
            assert(false);
        }
    }

    return ViewerGL::Implementation::POLYGON_PARTIAL;
} // getWipePolygon

ViewerGL::ViewerGL(ViewerTab* parent,
                   const QGLWidget* shareWidget)
    : QGLWidget(parent,shareWidget)
      , _imp( new Implementation(parent, this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    QObject::connect( parent->getGui()->getApp()->getProject().get(),SIGNAL( formatChanged(Format) ),this,SLOT( onProjectFormatChanged(Format) ) );



    _imp->blankViewerInfos.setChannels(Natron::Mask_RGBA);

    Format projectFormat;
    parent->getGui()->getApp()->getProject()->getProjectDefaultFormat(&projectFormat);

    _imp->blankViewerInfos.setRoD(projectFormat);
    _imp->blankViewerInfos.setDisplayWindow(projectFormat);
    setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),0);
    setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),1);
    onProjectFormatChanged(projectFormat);
    resetWipeControls();
    populateMenu();
    QObject::connect( getInternalNode(), SIGNAL( rodChanged(RectD, int) ), this, SLOT( setRegionOfDefinition(RectD, int) ) );
    QObject::connect( appPTR, SIGNAL(checkerboardSettingsChanged()), this, SLOT(onCheckerboardSettingsChanged()));
}

ViewerGL::~ViewerGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();

    if (_imp->shaderRGB) {
        _imp->shaderRGB->removeAllShaders();
        delete _imp->shaderRGB;
    }
    if (_imp->shaderBlack) {
        _imp->shaderBlack->removeAllShaders();
        delete _imp->shaderBlack;
    }
    delete _imp->displayTextures[0];
    delete _imp->displayTextures[1];
    glCheckError();
    for (U32 i = 0; i < _imp->pboIds.size(); ++i) {
        glDeleteBuffers(1,&_imp->pboIds[i]);
    }
    glCheckError();
    glDeleteBuffers(1, &_imp->vboVerticesId);
    glDeleteBuffers(1, &_imp->vboTexturesId);
    glDeleteBuffers(1, &_imp->iboTriangleStripId);
    glCheckError();
    delete _imp->textFont;
    glDeleteTextures(1, &_imp->checkerboardTextureID);
}

QSize
ViewerGL::sizeHint() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return QSize(1920,1080);
}

const QFont &
ViewerGL::textFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return *_imp->textFont;
}

void
ViewerGL::setTextFont(const QFont & f)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    *_imp->textFont = f;
}

/**
 *@returns Returns true if the viewer is displaying something.
 **/
bool
ViewerGL::displayingImage() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->activeTextures[0] != 0 || _imp->activeTextures[1] != 0;
}

void
ViewerGL::resizeGL(int width,
                   int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( (height == 0) || (width == 0) ) { // prevent division by 0
        return;
    }
    glViewport (0, 0, width, height);
    bool zoomSinceLastFit;
    int oldWidth,oldHeight;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        oldWidth = _imp->zoomCtx.screenWidth();
        oldHeight = _imp->zoomCtx.screenHeight();
        _imp->zoomCtx.setScreenSize(width, height);
        zoomSinceLastFit = _imp->zoomOrPannedSinceLastFit;
    }
    glCheckError();
    _imp->ms = UNDEFINED;
    assert(_imp->viewerTab);
    ViewerInstance* viewer = _imp->viewerTab->getInternalNode();
    assert(viewer);
    if (!zoomSinceLastFit) {
        fitImageToFormat();
    }
    if ( viewer->getUiContext() && _imp->viewerTab->getGui() &&
         !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject() &&
         ( ( oldWidth != width) || ( oldHeight != height) ) ) {
        viewer->renderCurrentFrame();
        updateGL();
    }
}

/**
 * @brief Used to setup the blending mode to draw the first texture
 **/
class BlendSetter
{
    
    bool didBlend;
    
public:
    
    BlendSetter(ImagePremultiplication premult)
    {
        didBlend = premult != ImageOpaque;
        if (didBlend) {
            glEnable(GL_BLEND);
        }
        switch (premult) {
            case Natron::ImagePremultiplied:
                glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
                break;
            case Natron::ImageUnPremultiplied:
                glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
                break;
            case Natron::ImageOpaque:
                break;
        }

    }
    
    ~BlendSetter()
    {
        if (didBlend) {
            glDisable(GL_BLEND);
        }
    }
};

void
ViewerGL::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //if app is closing, just return
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }
    glCheckError();

    {
        GLProtectAttrib a(GL_TRANSFORM_BIT);
        GLProtectMatrix m(GL_MODELVIEW);
        GLProtectMatrix p(GL_PROJECTION);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        double zoomLeft, zoomRight, zoomBottom, zoomTop;
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            assert(0 < _imp->zoomCtx.factor() && _imp->zoomCtx.factor() <= 1024);
            zoomLeft = _imp->zoomCtx.left();
            zoomRight = _imp->zoomCtx.right();
            zoomBottom = _imp->zoomCtx.bottom();
            zoomTop = _imp->zoomCtx.top();
        }
        if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
            clearColorBuffer( _imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF() );

            return;
        }

        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, -1, 1);
        glCheckError();



        // don't even bind the shader on 8-bits gamma-compressed textures
        ViewerCompositingOperator compOp = _imp->viewerTab->getCompositingOperator();

        ///Determine whether we need to draw each texture or not
        int activeInputs[2];
        _imp->viewerTab->getInternalNode()->getActiveInputs(activeInputs[0], activeInputs[1]);
        bool drawTexture[2];
        drawTexture[0] = _imp->activeTextures[0];
        drawTexture[1] = _imp->activeTextures[1] && compOp != OPERATOR_NONE;
        if ( (activeInputs[0] == activeInputs[1]) && (compOp != OPERATOR_MINUS) ) {
            drawTexture[1] = false;
        }
        double wipeMix;
        {
            QMutexLocker l(&_imp->wipeControlsMutex);
            wipeMix = _imp->mixAmount;
        }

        GLuint savedTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
        {
            GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

            clearColorBuffer( _imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF() );
            glCheckErrorIgnoreOSXBug();

            glEnable (GL_TEXTURE_2D);

            glBlendColor(1, 1, 1, wipeMix);
            
            ///Depending on the premultiplication of the input image we use a different blending func
            ImagePremultiplication premultA = _imp->displayingImagePremult[0];
            if (!_imp->viewerTab->isCheckerboardEnabled()) {
                premultA = Natron::ImageOpaque; ///When no checkerboard, draw opaque
            }

            if (compOp == OPERATOR_WIPE) {
                ///In wipe mode draw first the input A then only the portion we are interested in the input B

                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,ALL_PLANE);
                }
                if (drawTexture[1]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,1,WIPE_RIGHT_PLANE);
                    glDisable(GL_BLEND);
                }
            } else if (compOp == OPERATOR_MINUS) {
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,ALL_PLANE);
                }
                if (drawTexture[1]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,1,WIPE_RIGHT_PLANE);
                    glDisable(GL_BLEND);
                }
            } else if (compOp == OPERATOR_UNDER) {
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,ALL_PLANE);
                }
                if (drawTexture[1]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,1,WIPE_RIGHT_PLANE);
                    glDisable(GL_BLEND);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,1,WIPE_RIGHT_PLANE);
                    glDisable(GL_BLEND);
                }
            } else if (compOp == OPERATOR_OVER) {
                ///draw first B then A
                if (drawTexture[1]) {
                    ///Depending on the premultiplication of the input image we use a different blending func
                    ImagePremultiplication premultB = _imp->displayingImagePremult[1];
                    if (!_imp->viewerTab->isCheckerboardEnabled()) {
                        premultB = Natron::ImageOpaque; ///When no checkerboard, draw opaque
                    }
                    BlendSetter b(premultB);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,1,WIPE_RIGHT_PLANE);
                }
                if (drawTexture[0]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,WIPE_RIGHT_PLANE);
                    glDisable(GL_BLEND);

                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,WIPE_LEFT_PLANE);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,WIPE_RIGHT_PLANE);
                    glDisable(GL_BLEND);
                }
            } else {
                if (drawTexture[0]) {
                    
                    BlendSetter b(premultA);
                    drawRenderingVAO(_imp->displayingImageMipMapLevel,0,ALL_PLANE);

                }
            }
        } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        ///Unbind render textures for overlays
        glBindTexture(GL_TEXTURE_2D, savedTexture);
        
        glCheckError();
        if (_imp->overlay) {
            drawOverlay(_imp->displayingImageMipMapLevel);
        }
        
        if (_imp->displayPersistentMessage) {
            drawPersistentMessage();
        }
        
        if (_imp->ms == SELECTING) {
            _imp->drawSelectionRectangle();
        }
        glCheckErrorAssert();
    } // GLProtectAttrib a(GL_TRANSFORM_BIT);
} // paintGL

void
ViewerGL::clearColorBuffer(double r,
                           double g,
                           double b,
                           double a )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    {
        GLProtectAttrib att(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

        glClearColor(r,g,b,a);
        glClear(GL_COLOR_BUFFER_BIT);
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
}

void
ViewerGL::toggleOverlays()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->overlay = !_imp->overlay;
    updateGL();
}

void
ViewerGL::drawOverlay(unsigned int mipMapLevel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    glCheckError();
    RectD dispW = getDisplayWindow();

    renderText(dispW.right(),dispW.bottom(), _imp->currentViewerInfos_resolutionOverlay,_imp->textRenderingColor,*_imp->textFont);


    QPoint topRight( dispW.right(),dispW.top() );
    QPoint topLeft( dispW.left(),dispW.top() );
    QPoint btmLeft( dispW.left(),dispW.bottom() );
    QPoint btmRight( dispW.right(),dispW.bottom() );

    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        glDisable(GL_BLEND);

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
        glCheckErrorIgnoreOSXBug();


        for (int i = 0; i < 2; ++i) {
            if (!_imp->activeTextures[i]) {
                break;
            }
            RectD dataW = getRoD(i);

            if (dataW != dispW) {
                renderText(dataW.right(), dataW.top(),
                           _imp->currentViewerInfos_topRightBBOXoverlay[i], _imp->rodOverlayColor,*_imp->textFont);
                renderText(dataW.left(), dataW.bottom(),
                           _imp->currentViewerInfos_btmLeftBBOXoverlay[i], _imp->rodOverlayColor,*_imp->textFont);


                QPoint topRight2( dataW.right(), dataW.top() );
                QPoint topLeft2( dataW.left(),dataW.top() );
                QPoint btmLeft2( dataW.left(),dataW.bottom() );
                QPoint btmRight2( dataW.right(),dataW.bottom() );
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
                glCheckError();
            }
        }

        bool userRoIEnabled;
        {
            QMutexLocker l(&_imp->userRoIMutex);
            userRoIEnabled = _imp->userRoIEnabled;
        }
        if (userRoIEnabled) {
            drawUserRoI();
        }

        ViewerCompositingOperator compOperator = _imp->viewerTab->getCompositingOperator();
        if (compOperator != OPERATOR_NONE) {
            drawWipeControl();
        }


        glCheckError();
        glColor4f(1., 1., 1., 1.);
        _imp->viewerTab->drawOverlays(1 << mipMapLevel,1 << mipMapLevel);
        glCheckError();

        if (_imp->pickerState == PICKER_RECTANGLE) {
            if ( _imp->viewerTab->getGui()->hasPickers() ) {
                drawPickerRectangle();
            }
        } else if (_imp->pickerState == PICKER_POINT) {
            if ( _imp->viewerTab->getGui()->hasPickers() ) {
                drawPickerPixel();
            }
        }

    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError();
} // drawOverlay

void
ViewerGL::drawUserRoI()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        glDisable(GL_BLEND);

        glColor4f(0.9, 0.9, 0.9, 1.);

        double zoomScreenPixelWidth, zoomScreenPixelHeight;
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
            zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
        }
        RectD userRoI;
        {
            QMutexLocker l(&_imp->userRoIMutex);
            userRoI = _imp->userRoI;
        }

        ///base rect
        glBegin(GL_LINE_LOOP);
        glVertex2f(userRoI.x1, userRoI.y1); //bottom left
        glVertex2f(userRoI.x1, userRoI.y2); //top left
        glVertex2f(userRoI.x2, userRoI.y2); //top right
        glVertex2f(userRoI.x2, userRoI.y1); //bottom right
        glEnd();


        glBegin(GL_LINES);
        ///border ticks
        double borderTickWidth = USER_ROI_BORDER_TICK_SIZE * zoomScreenPixelWidth;
        double borderTickHeight = USER_ROI_BORDER_TICK_SIZE * zoomScreenPixelHeight;
        glVertex2f(userRoI.x1, (userRoI.y1 + userRoI.y2) / 2);
        glVertex2f(userRoI.x1 - borderTickWidth, (userRoI.y1 + userRoI.y2) / 2);

        glVertex2f(userRoI.x2, (userRoI.y1 + userRoI.y2) / 2);
        glVertex2f(userRoI.x2 + borderTickWidth, (userRoI.y1 + userRoI.y2) / 2);

        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y2 );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y2 + borderTickHeight );

        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y1 );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y1 - borderTickHeight );

        ///middle cross
        double crossWidth = USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth;
        double crossHeight = USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight;
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2 - crossHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2 + crossHeight );

        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2  - crossWidth, (userRoI.y1 + userRoI.y2) / 2 );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2  + crossWidth, (userRoI.y1 + userRoI.y2) / 2 );
        glEnd();


        ///draw handles hint for the user
        glBegin(GL_QUADS);

        double rectHalfWidth = (USER_ROI_SELECTION_POINT_SIZE * zoomScreenPixelWidth) / 2.;
        double rectHalfHeight = (USER_ROI_SELECTION_POINT_SIZE * zoomScreenPixelWidth) / 2.;
        //left
        glVertex2f(userRoI.x1 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
        glVertex2f(userRoI.x1 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        glVertex2f(userRoI.x1 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        glVertex2f(userRoI.x1 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);

        //top
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y2 - rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y2 + rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y2 + rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y2 - rectHalfHeight );

        //right
        glVertex2f(userRoI.x2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
        glVertex2f(userRoI.x2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        glVertex2f(userRoI.x2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        glVertex2f(userRoI.x2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);

        //bottom
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y1 - rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y1 + rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y1 + rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y1 - rectHalfHeight );

        //middle
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight );
        glVertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight );


        //top left
        glVertex2f(userRoI.x1 - rectHalfWidth, userRoI.y2 - rectHalfHeight);
        glVertex2f(userRoI.x1 - rectHalfWidth, userRoI.y2 + rectHalfHeight);
        glVertex2f(userRoI.x1 + rectHalfWidth, userRoI.y2 + rectHalfHeight);
        glVertex2f(userRoI.x1 + rectHalfWidth, userRoI.y2 - rectHalfHeight);

        //top right
        glVertex2f(userRoI.x2 - rectHalfWidth, userRoI.y2 - rectHalfHeight);
        glVertex2f(userRoI.x2 - rectHalfWidth, userRoI.y2 + rectHalfHeight);
        glVertex2f(userRoI.x2 + rectHalfWidth, userRoI.y2 + rectHalfHeight);
        glVertex2f(userRoI.x2 + rectHalfWidth, userRoI.y2 - rectHalfHeight);

        //bottom right
        glVertex2f(userRoI.x2 - rectHalfWidth, userRoI.y1 - rectHalfHeight);
        glVertex2f(userRoI.x2 - rectHalfWidth, userRoI.y1 + rectHalfHeight);
        glVertex2f(userRoI.x2 + rectHalfWidth, userRoI.y1 + rectHalfHeight);
        glVertex2f(userRoI.x2 + rectHalfWidth, userRoI.y1 - rectHalfHeight);
        
        
        //bottom left
        glVertex2f(userRoI.x1 - rectHalfWidth, userRoI.y1 - rectHalfHeight);
        glVertex2f(userRoI.x1 - rectHalfWidth, userRoI.y1 + rectHalfHeight);
        glVertex2f(userRoI.x1 + rectHalfWidth, userRoI.y1 + rectHalfHeight);
        glVertex2f(userRoI.x1 + rectHalfWidth, userRoI.y1 - rectHalfHeight);
        
        glEnd();
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
} // drawUserRoI

void
ViewerGL::drawWipeControl()
{
    double wipeAngle;
    QPointF wipeCenter;
    double mixAmount;
    {
        QMutexLocker l(&_imp->wipeControlsMutex);
        wipeAngle = _imp->wipeAngle;
        wipeCenter = _imp->wipeCenter;
        mixAmount = _imp->mixAmount;
    }
    double alphaMix1,alphaMix0,alphaCurMix;
    double mpi8 = M_PI / 8;

    alphaMix1 = wipeAngle + mpi8;
    alphaMix0 = wipeAngle + 3. * mpi8;
    alphaCurMix = mixAmount * (alphaMix1 - alphaMix0) + alphaMix0;
    QPointF mix0Pos,mixPos,mix1Pos;
    double mixLength,rotateLenght,rotateOffset,zoomFactor;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomFactor = _imp->zoomCtx.factor();
    }
    mixLength = WIPE_MIX_HANDLE_LENGTH / zoomFactor;
    rotateLenght = WIPE_ROTATE_HANDLE_LENGTH / zoomFactor,
    rotateOffset = WIPE_ROTATE_OFFSET / zoomFactor;


    mixPos.setX(wipeCenter.x() + std::cos(alphaCurMix) * mixLength);
    mixPos.setY(wipeCenter.y() + std::sin(alphaCurMix) * mixLength);
    mix0Pos.setX(wipeCenter.x() + std::cos(alphaMix0) * mixLength);
    mix0Pos.setY(wipeCenter.y() + std::sin(alphaMix0) * mixLength);
    mix1Pos.setX(wipeCenter.x() + std::cos(alphaMix1) * mixLength);
    mix1Pos.setY(wipeCenter.y() + std::sin(alphaMix1) * mixLength);

    QPointF oppositeAxisBottom,oppositeAxisTop,rotateAxisLeft,rotateAxisRight;
    rotateAxisRight.setX( wipeCenter.x() + std::cos(wipeAngle) * (rotateLenght - rotateOffset) );
    rotateAxisRight.setY( wipeCenter.y() + std::sin(wipeAngle) * (rotateLenght - rotateOffset) );
    rotateAxisLeft.setX(wipeCenter.x() - std::cos(wipeAngle) * rotateOffset);
    rotateAxisLeft.setY( wipeCenter.y() - (std::sin(wipeAngle) * rotateOffset) );

    oppositeAxisTop.setX( wipeCenter.x() + std::cos(wipeAngle + M_PI / 2.) * (rotateLenght / 2.) );
    oppositeAxisTop.setY( wipeCenter.y() + std::sin(wipeAngle + M_PI / 2.) * (rotateLenght / 2.) );
    oppositeAxisBottom.setX( wipeCenter.x() - std::cos(wipeAngle + M_PI / 2.) * (rotateLenght / 2.) );
    oppositeAxisBottom.setY( wipeCenter.y() - std::sin(wipeAngle + M_PI / 2.) * (rotateLenght / 2.) );

    {
        GLProtectAttrib a(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);
        GLProtectMatrix p(GL_PROJECTION);

        // Draw everything twice
        // l = 0: shadow
        // l = 1: drawing
        double baseColor[3];
        for (int l = 0; l < 2; ++l) {
            if (l == 0) {
                // Draw a shadow for the cross hair
                // shift by (1,1) pixel
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glTranslated(1. / zoomFactor, -1. / zoomFactor, 0);
                baseColor[0] = baseColor[1] = baseColor[2] = 0.;
            } else {
                baseColor[0] = baseColor[1] = baseColor[2] = 0.8;
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(1.5);
            glBegin(GL_LINES);
            if ( (_imp->hs == HOVERING_WIPE_ROTATE_HANDLE) || (_imp->ms == ROTATING_WIPE_HANDLE) ) {
                glColor4f(0., 1., 0., 1.);
            }
            glColor4f(baseColor[0],baseColor[1],baseColor[2],1.);
            glVertex2d( rotateAxisLeft.x(), rotateAxisLeft.y() );
            glVertex2d( rotateAxisRight.x(), rotateAxisRight.y() );
            glVertex2d( oppositeAxisBottom.x(), oppositeAxisBottom.y() );
            glVertex2d( oppositeAxisTop.x(), oppositeAxisTop.y() );
            glVertex2d( wipeCenter.x(),wipeCenter.y() );
            glVertex2d( mixPos.x(), mixPos.y() );
            glEnd();
            glLineWidth(1.);

            ///if hovering the rotate handle or dragging it show a small bended arrow
            if ( (_imp->hs == HOVERING_WIPE_ROTATE_HANDLE) || (_imp->ms == ROTATING_WIPE_HANDLE) ) {
                GLProtectMatrix p(GL_PROJECTION);

                glColor4f(0., 1., 0., 1.);
                double arrowCenterX = WIPE_ROTATE_HANDLE_LENGTH / (2. * zoomFactor);
                ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
                OfxPointD arrowRadius;
                arrowRadius.x = 5. / zoomFactor;
                arrowRadius.y = 10. / zoomFactor;

                glMatrixMode(GL_PROJECTION);
                glTranslatef(wipeCenter.x(), wipeCenter.y(), 0.);
                glRotatef(wipeAngle * 180.0 / M_PI,0, 0, 1);
                //  center the oval at x_center, y_center
                glTranslatef (arrowCenterX, 0., 0);
                //  draw the oval using line segments
                glBegin (GL_LINE_STRIP);
                glVertex2f (0, arrowRadius.y);
                glVertex2f (arrowRadius.x, 0.);
                glVertex2f (0, -arrowRadius.y);
                glEnd ();


                glBegin(GL_LINES);
                ///draw the top head
                glVertex2f(0., arrowRadius.y);
                glVertex2f(0., arrowRadius.y -  arrowRadius.x );

                glVertex2f(0., arrowRadius.y);
                glVertex2f(4. / zoomFactor, arrowRadius.y - 3. / zoomFactor); // 5^2 = 3^2+4^2

                ///draw the bottom head
                glVertex2f(0., -arrowRadius.y);
                glVertex2f(0., -arrowRadius.y + 5. / zoomFactor);

                glVertex2f(0., -arrowRadius.y);
                glVertex2f(4. / zoomFactor, -arrowRadius.y + 3. / zoomFactor); // 5^2 = 3^2+4^2

                glEnd();

                glColor4f(baseColor[0],baseColor[1],baseColor[2],1.);
            }

            glPointSize(5.);
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2d( wipeCenter.x(), wipeCenter.y() );
            if ( ( (_imp->hs == HOVERING_WIPE_MIX) && (_imp->ms != ROTATING_WIPE_HANDLE) ) || (_imp->ms == DRAGGING_WIPE_MIX_HANDLE) ) {
                glColor4f(0., 1., 0., 1.);
            }
            glVertex2d( mixPos.x(), mixPos.y() );
            glEnd();
            glPointSize(1.);
            
            _imp->drawArcOfCircle(wipeCenter, mixLength, wipeAngle + M_PI / 8., wipeAngle + 3. * M_PI / 8.);
            if (l == 0) {
                glMatrixMode(GL_PROJECTION);
                glPopMatrix();
            }
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);
} // drawWipeControl

void
ViewerGL::Implementation::drawArcOfCircle(const QPointF & center,
                                          double radius,
                                          double startAngle,
                                          double endAngle)
{
    double alpha = startAngle;
    double x,y;

    {
        GLProtectAttrib a(GL_CURRENT_BIT);

        if ( (hs == HOVERING_WIPE_MIX) || (ms == DRAGGING_WIPE_MIX_HANDLE) ) {
            glColor3f(0, 1, 0);
        }
        glBegin(GL_POINTS);
        while (alpha <= endAngle) {
            x = center.x()  + radius * std::cos(alpha);
            y = center.y()  + radius * std::sin(alpha);
            glVertex2d(x, y);
            alpha += 0.01;
        }
        glEnd();
    } // GLProtectAttrib a(GL_CURRENT_BIT);
}

void
ViewerGL::drawPickerRectangle()
{
    {
        GLProtectAttrib a(GL_CURRENT_BIT);

        glColor3f(0.9, 0.7, 0.);
        QPointF topLeft = _imp->pickerRect.topLeft();
        QPointF btmRight = _imp->pickerRect.bottomRight();
        ///base rect
        glBegin(GL_LINE_LOOP);
        glVertex2f( topLeft.x(), btmRight.y() ); //bottom left
        glVertex2f( topLeft.x(), topLeft.y() ); //top left
        glVertex2f( btmRight.x(), topLeft.y() ); //top right
        glVertex2f( btmRight.x(), btmRight.y() ); //bottom right
        glEnd();
    } // GLProtectAttrib a(GL_CURRENT_BIT);
}

void
ViewerGL::drawPickerPixel()
{
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POINT_SMOOTH);
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            glPointSize( 1. * _imp->zoomCtx.factor() );
        }

        QPointF pos = _imp->lastPickerPos;
        unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();

        if (mipMapLevel != 0) {
            pos *= (1 << mipMapLevel);
        }
        glColor3f(0.9, 0.7, 0.);
        glBegin(GL_POINTS);
        glVertex2d( pos.x(),pos.y() );
        glEnd();
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT);
}

void
ViewerGL::drawPersistentMessage()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    QFontMetrics metrics( font() );
    int numberOfLines = std::ceil( (double)metrics.width(_imp->persistentMessage) / (double)(width() - 20) );
    int averageCharsPerLine = numberOfLines != 0 ? _imp->persistentMessage.size() / numberOfLines : _imp->persistentMessage.size();
    QStringList lines;
    int i = 0;
    QString message = _imp->persistentMessage;
    while (!message.isEmpty()) {
        QString str;
        while ( i < message.size() ) {
            if ( (i % averageCharsPerLine == 0) && (i != 0) ) {
                break;
            }
            str.append( message.at(i) );
            ++i;
        }
        
        if (i < message.size()) {
            int originalIndex = i;
            /*Find closest word before and insert a new line*/
            while ( i >= 0 && message.at(i) != QChar(' ') && message.at(i) != QChar('\t') &&
                   message.at(i) != QChar('\n')) {
                --i;
            }
            if (i >= 0) {
                str.remove(i, str.size() - originalIndex);
            }
        }
        message.remove(0, str.size());
        lines.append(str);
        i = 0;
    }

    int offset = metrics.height() + 10;
    QPointF topLeft, bottomRight, textPos;
    double zoomScreenPixelHeight;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        topLeft = _imp->zoomCtx.toZoomCoordinates(0,0);
        bottomRight = _imp->zoomCtx.toZoomCoordinates( _imp->zoomCtx.screenWidth(),numberOfLines * (metrics.height() * 2) );
        textPos = _imp->zoomCtx.toZoomCoordinates(20, offset);
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    
    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glDisable(GL_BLEND);

        if (_imp->persistentMessageType == 1) { // error
            glColor4f(0.5,0.,0.,1.);
        } else { // warning
            glColor4f(0.65,0.65,0.,1.);
        }
        glBegin(GL_POLYGON);
        glVertex2f( topLeft.x(),topLeft.y() ); //top left
        glVertex2f( topLeft.x(),bottomRight.y() ); //bottom left
        glVertex2f( bottomRight.x(),bottomRight.y() ); //bottom right
        glVertex2f( bottomRight.x(),topLeft.y() ); //top right
        glEnd();


        for (int j = 0; j < lines.size(); ++j) {
            renderText(textPos.x(),textPos.y(), lines.at(j),_imp->textRenderingColor,*_imp->textFont);
            textPos.setY(textPos.y() - metrics.height() * 2 * zoomScreenPixelHeight);
        }
        glCheckError();
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
} // drawPersistentMessage

void
ViewerGL::Implementation::drawSelectionRectangle()
{
    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);

        glColor4f(0.5,0.8,1.,0.4);
        QPointF btmRight = selectionRectangle.bottomRight();
        QPointF topLeft = selectionRectangle.topLeft();

        glBegin(GL_POLYGON);
        glVertex2f( topLeft.x(),btmRight.y() );
        glVertex2f( topLeft.x(),topLeft.y() );
        glVertex2f( btmRight.x(),topLeft.y() );
        glVertex2f( btmRight.x(),btmRight.y() );
        glEnd();


        glLineWidth(1.5);

        glBegin(GL_LINE_LOOP);
        glVertex2f( topLeft.x(),btmRight.y() );
        glVertex2f( topLeft.x(),topLeft.y() );
        glVertex2f( btmRight.x(),topLeft.y() );
        glVertex2f( btmRight.x(),btmRight.y() );
        glEnd();

        glCheckError();
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
}

void
ViewerGL::Implementation::drawCheckerboardTexture(const RectD& rod)
{
    ///We divide by 2 the tiles count because one texture is 4 tiles actually
    QPointF topLeft,btmRight;
    double screenW,screenH;
    QPointF rodBtmLeft;
    QPointF rodTopRight;
    {
        QMutexLocker l(&zoomCtxMutex);
        topLeft = zoomCtx.toZoomCoordinates(0, 0);
        screenW = zoomCtx.screenWidth();
        screenH = zoomCtx.screenHeight();
        btmRight = zoomCtx.toZoomCoordinates(screenW - 1, screenH - 1);
        rodBtmLeft = zoomCtx.toWidgetCoordinates(rod.x1, rod.y1);
        rodTopRight = zoomCtx.toWidgetCoordinates(rod.x2, rod.y2);
    }
    
    double xTilesCountF = screenW / (checkerboardTileSize * 4); //< 4 because the texture contains 4 tiles
    double yTilesCountF = screenH / (checkerboardTileSize * 4);

    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib a(GL_SCISSOR_BIT | GL_ENABLE_BIT);

        glEnable(GL_SCISSOR_TEST);
        glScissor(rodBtmLeft.x(), screenH - rodBtmLeft.y(), rodTopRight.x() - rodBtmLeft.x(), rodBtmLeft.y() - rodTopRight.y());

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, checkerboardTextureID);
        glBegin(GL_POLYGON);
        glTexCoord2d(0., 0.); glVertex2d(topLeft.x(),btmRight.y());
        glTexCoord2d(0., yTilesCountF); glVertex2d(topLeft.x(),topLeft.y());
        glTexCoord2d(xTilesCountF, yTilesCountF); glVertex2d(btmRight.x(), topLeft.y());
        glTexCoord2d(xTilesCountF, 0.); glVertex2d(btmRight.x(), btmRight.y());
        glEnd();


        //glDisable(GL_SCISSOR_TEST);
    } // GLProtectAttrib a(GL_SCISSOR_BIT | GL_ENABLE_BIT);
    glBindTexture(GL_TEXTURE_2D, savedTexture);
    glCheckError();
}

void
ViewerGL::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();
    initAndCheckGlExtensions();
    _imp->displayTextures[0] = new Texture(GL_TEXTURE_2D,GL_LINEAR,GL_NEAREST);
    _imp->displayTextures[1] = new Texture(GL_TEXTURE_2D,GL_LINEAR,GL_NEAREST);


    // glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &_imp->vboVerticesId);
    glGenBuffers(1, &_imp->vboTexturesId);
    glGenBuffers(1, &_imp->iboTriangleStripId);

    glBindBuffer(GL_ARRAY_BUFFER, _imp->vboTexturesId);
    glBufferData(GL_ARRAY_BUFFER, 32 * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _imp->vboVerticesId);
    glBufferData(GL_ARRAY_BUFFER, 32 * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _imp->iboTriangleStripId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 28 * sizeof(GLubyte), triangleStrip, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError();

    _imp->initializeCheckerboardTexture(true);
    
    if (_imp->supportsGLSL) {
        initShaderGLSL();
        glCheckError();
    }

    glCheckError();
}

void
ViewerGL::Implementation::initializeCheckerboardTexture(bool mustCreateTexture)
{
    if (mustCreateTexture) {
        glGenTextures(1, &checkerboardTextureID);
    }
    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib a(GL_ENABLE_BIT);

        glEnable(GL_TEXTURE_2D);
        glBindTexture (GL_TEXTURE_2D,checkerboardTextureID);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        double color1[4];
        double color2[4];
        appPTR->getCurrentSettings()->getCheckerboardColor1(&color1[0], &color1[1], &color1[2], &color1[3]);
        appPTR->getCurrentSettings()->getCheckerboardColor2(&color2[0], &color2[1], &color2[2], &color2[3]);

        unsigned char checkerboardTexture[16];
        ///Fill the first line
        for (int i = 0; i < 4; ++i) {
            checkerboardTexture[i] = Color::floatToInt<256>(color1[i]);
            checkerboardTexture[i + 4] = Color::floatToInt<256>(color2[i]);
        }
        ///Copy the first line to the second line
        memcpy(&checkerboardTexture[8], &checkerboardTexture[4], sizeof(unsigned char) * 4);
        memcpy(&checkerboardTexture[12], &checkerboardTexture[0], sizeof(unsigned char) * 4);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, (void*)checkerboardTexture);
    } // GLProtectAttrib a(GL_ENABLE_BIT);
    glBindTexture(GL_TEXTURE_2D, savedTexture);
    
    checkerboardTileSize = appPTR->getCurrentSettings()->getCheckerboardTileSize();

    
   
}

QString
ViewerGL::getOpenGLVersionString() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    const char* str = (const char*)glGetString(GL_VERSION);
    QString ret;
    if (str) {
        ret.append(str);
    }

    return ret;
}

QString
ViewerGL::getGlewVersionString() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    const char* str = reinterpret_cast<const char *>( glewGetString(GLEW_VERSION) );
    QString ret;
    if (str) {
        ret.append(str);
    }

    return ret;
}

GLuint
ViewerGL::getPboID(int index)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( index >= (int)_imp->pboIds.size() ) {
        GLuint handle;
        glGenBuffers(1,&handle);
        _imp->pboIds.push_back(handle);

        return handle;
    } else {
        return _imp->pboIds[index];
    }
}

/**
 *@returns Returns the current zoom factor that is applied to the display.
 **/
double
ViewerGL::getZoomFactor() const
{
    // MT-SAFE
    QMutexLocker l(&_imp->zoomCtxMutex);

    return _imp->zoomCtx.factor();
}

///imageRoD is in PIXEL COORDINATES
RectI
ViewerGL::getImageRectangleDisplayed(const RectI & imageRoDPixel, // in pixel coordinates
                                     unsigned int mipMapLevel)
{
    // MT-SAFE
    RectD visibleArea;
    RectI ret;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF topLeft =  _imp->zoomCtx.toZoomCoordinates(0, 0);
        visibleArea.x1 =  topLeft.x();
        visibleArea.y2 =  topLeft.y();
        QPointF bottomRight = _imp->zoomCtx.toZoomCoordinates(width() - 1, height() - 1);
        visibleArea.x2 = bottomRight.x() ;
        visibleArea.y1 = bottomRight.y();
    }

    if (mipMapLevel != 0) {
        // for the viewer, we need the smallest enclosing rectangle at the mipmap level, in order to avoid black borders
        visibleArea.toPixelEnclosing(mipMapLevel,&ret);
    } else {
        ret.x1 = visibleArea.x1;
        ret.x2 = visibleArea.x2;
        ret.y1 = visibleArea.y1;
        ret.y2 = visibleArea.y2;
    }

    ///If the roi doesn't intersect the image's Region of Definition just return an empty rectangle
    if ( !ret.intersect(imageRoDPixel, &ret) ) {
        ret.clear();
    }

    ///to clip against the user roi however clip it against the mipmaplevel of the zoomFactor+proxy

    RectD userRoI;
    bool userRoiEnabled;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoiEnabled = _imp->userRoIEnabled;
        userRoI = _imp->userRoI;
    }
    if (userRoiEnabled) {
        RectI userRoIpixel;

        ///If the user roi is enabled, we want to render the smallest enclosing rectangle in order to avoid black borders.
        userRoI.toPixelEnclosing(mipMapLevel, &userRoIpixel);

        ///If the user roi doesn't intersect the actually visible portion on the viewer, return an empty rectangle.
        if ( !ret.intersect(userRoIpixel, &ret) ) {
            ret.clear();
        }
    }

    return ret;
}

int
ViewerGL::isExtensionSupported(const char *extension)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;
    where = (GLubyte *) strchr(extension, ' ');
    if ( where || (*extension == '\0') ) {
        return 0;
    }
    extensions = glGetString(GL_EXTENSIONS);
    start = extensions;
    for (;; ) {
        where = (GLubyte *) strstr( (const char *) start, extension );
        if (!where) {
            break;
        }
        terminator = where + strlen(extension);
        if ( (where == start) || (*(where - 1) == ' ') ) {
            if ( (*terminator == ' ') || (*terminator == '\0') ) {
                return 1;
            }
        }
        start = terminator;
    }

    return 0;
}

void
ViewerGL::initAndCheckGlExtensions()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        Natron::errorDialog( tr("OpenGL/GLEW error").toStdString(),
                             (const char*)glewGetErrorString(err) );
    }
    //fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    // is GL_VERSION_2_0 necessary? note that GL_VERSION_2_0 includes GLSL
    if ( !glewIsSupported("GL_VERSION_1_5 "
                          "GL_ARB_texture_non_power_of_two " // or GL_IMG_texture_npot, or GL_OES_texture_npot, core since 2.0
                          "GL_ARB_shader_objects " // GLSL, Uniform*, core since 2.0
                          "GL_ARB_vertex_buffer_object " // BindBuffer, MapBuffer, etc.
                          "GL_ARB_pixel_buffer_object " // BindBuffer(PIXEL_UNPACK_BUFFER,...
                          //"GL_ARB_vertex_array_object " // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                          //"GL_ARB_framebuffer_object " // or GL_EXT_framebuffer_object GenFramebuffers, core since version 3.0
                          ) ) {
        Natron::errorDialog( tr("Missing OpenGL requirements").toStdString(),
                             tr("The viewer may not be fully functional. "
                                "This software needs at least OpenGL 1.5 with NPOT textures, GLSL, VBO, PBO, vertex arrays. ").toStdString() );
    }

    _imp->viewerTab->getGui()->setOpenGLVersion( getOpenGLVersionString() );
    _imp->viewerTab->getGui()->setGlewVersion( getGlewVersionString() );

    if ( !QGLShaderProgram::hasOpenGLShaderPrograms( context() ) ) {
        // no need to pull out a dialog, it was already presented after the GLEW check above

        //Natron::errorDialog("Viewer error","The viewer is unabgile to work without a proper version of GLSL.");
        //cout << "Warning : GLSL not present on this hardware, no material acceleration possible." << endl;
        _imp->supportsGLSL = false;
    }
}

void
ViewerGL::Implementation::activateShaderRGB(int texIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // we assume that:
    // - 8-bits textures are stored non-linear and must be displayer as is
    // - floating-point textures are linear and must be decompressed according to the given lut

    assert(supportsGLSL);

    if ( !shaderRGB->bind() ) {
        cout << qPrintable( shaderRGB->log() ) << endl;
    }

    shaderRGB->setUniformValue("Tex", 0);
    shaderRGB->setUniformValue("gain", (float)displayingImageGain[texIndex]);
    shaderRGB->setUniformValue("offset", (float)displayingImageOffset[texIndex]);
    shaderRGB->setUniformValue("lut", (GLint)displayingImageLut);
}

void
ViewerGL::initShaderGLSL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if (!_imp->shaderLoaded && _imp->supportsGLSL) {
        _imp->shaderBlack = new QGLShaderProgram( context() );
        if ( !_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Vertex,vertRGB) ) {
            cout << qPrintable( _imp->shaderBlack->log() ) << endl;
        }
        if ( !_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Fragment,blackFrag) ) {
            cout << qPrintable( _imp->shaderBlack->log() ) << endl;
        }
        if ( !_imp->shaderBlack->link() ) {
            cout << qPrintable( _imp->shaderBlack->log() ) << endl;
        }

        _imp->shaderRGB = new QGLShaderProgram( context() );
        if ( !_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Vertex,vertRGB) ) {
            cout << qPrintable( _imp->shaderRGB->log() ) << endl;
        }
        if ( !_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Fragment,fragRGB) ) {
            cout << qPrintable( _imp->shaderRGB->log() ) << endl;
        }

        if ( !_imp->shaderRGB->link() ) {
            cout << qPrintable( _imp->shaderRGB->log() ) << endl;
        }
        _imp->shaderLoaded = true;
    }
}



void
ViewerGL::transferBufferFromRAMtoGPU(const unsigned char* ramBuffer,
                                     const boost::shared_ptr<Natron::Image>& image,
                                     size_t bytesCount,
                                     const TextureRect & region,
                                     double gain,
                                     double offset,
                                     int lut,
                                     int pboIndex,
                                     unsigned int mipMapLevel,
                                     Natron::ImagePremultiplication premult,
                                     int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    (void)glGetError();
    GLint currentBoundPBO = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    GLenum err = glGetError();
    if ( (err != GL_NO_ERROR) || (currentBoundPBO != 0) ) {
        qDebug() << "(ViewerGL::allocateAndMapPBO): Another PBO is currently mapped, glMap failed." << endl;
    }

    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, getPboID(pboIndex) );
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);
    GLvoid *ret = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError();
    assert(ret);

    memcpy(ret, (void*)ramBuffer, bytesCount);

    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glCheckError();

    OpenGLViewerI::BitDepth bd = getBitDepth();
    assert(textureIndex == 0 || textureIndex == 1);
    if (bd == OpenGLViewerI::BYTE) {
        _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region,Texture::BYTE);
    } else if ( (bd == OpenGLViewerI::FLOAT) || (bd == OpenGLViewerI::HALF_FLOAT) ) {
        //do 32bit fp textures either way, don't bother with half float. We might support it further on.
        _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region,Texture::FLOAT);
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);
    //glBindTexture(GL_TEXTURE_2D, 0); // why should we bind texture 0?
    glCheckError();
    _imp->activeTextures[textureIndex] = _imp->displayTextures[textureIndex];
    _imp->displayingImageGain[textureIndex] = gain;
    _imp->displayingImageOffset[textureIndex] = offset;
    _imp->displayingImageMipMapLevel = mipMapLevel;
    _imp->displayingImageLut = (Natron::ViewerColorSpace)lut;
    _imp->displayingImagePremult[textureIndex] = premult;
    
    ViewerInstance* internalNode = getInternalNode();
    
    if (_imp->memoryHeldByLastRenderedImages[textureIndex] > 0) {
        internalNode->unregisterPluginMemory(_imp->memoryHeldByLastRenderedImages[textureIndex]);
        _imp->memoryHeldByLastRenderedImages[textureIndex] = 0;
    }
    
    
    if (image) {
        _imp->lastRenderedImage[textureIndex] = image;
        _imp->memoryHeldByLastRenderedImages[textureIndex] = image->size();
        internalNode->registerPluginMemory(_imp->memoryHeldByLastRenderedImages[textureIndex]);
    }
    updateGL();
    emit imageChanged(textureIndex);
}

void
ViewerGL::disconnectInputTexture(int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(textureIndex == 0 || textureIndex == 1);
    _imp->activeTextures[textureIndex] = 0;
}

void
ViewerGL::setGain(double d)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->displayingImageGain[0] = d;
    _imp->displayingImageGain[1] = d;
}

void
ViewerGL::setLut(int lut)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->displayingImageLut = (Natron::ViewerColorSpace)lut;
}

/**
 *@returns Returns true if the graphic card supports GLSL.
 **/
bool
ViewerGL::supportsGLSL() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->supportsGLSL;
}

#if QT_VERSION < 0x050000
#define QMouseEventLocalPos(e) ( e->posF() )
#else
#define QMouseEventLocalPos(e) ( e->localPos() )
#endif

void
ViewerGL::mousePressEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }
    
    ///Set focus on user click
    setFocus();
    
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::MouseButton button = e->button();

    if ( buttonDownIsLeft(e) ) {
        _imp->viewerTab->getGui()->selectNode( _imp->viewerTab->getGui()->getApp()->getNodeGui( _imp->viewerTab->getInternalNode()->getNode() ) );
    }

    _imp->oldClick = e->pos();
    _imp->lastMousePosition = e->pos();
    QPointF zoomPos;
    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    RectD userRoI;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoI = _imp->userRoI;
    }
    bool overlaysCaught = false;
    bool mustRedraw = false;
    double wipeSelectionTol;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        wipeSelectionTol = 8. / _imp->zoomCtx.factor();
    }

    if ( buttonDownIsMiddle(e) && !modifierHasControl(e) ) {
        _imp->ms = DRAGGING_IMAGE;
        overlaysCaught = true;
    } else if ( (_imp->ms == UNDEFINED) && _imp->overlay ) {
        unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
        overlaysCaught = _imp->viewerTab->notifyOverlaysPenDown(1 << mipMapLevel, 1 << mipMapLevel, QMouseEventLocalPos(e), zoomPos, e);
        if (overlaysCaught) {
            mustRedraw = true;
        }
    }


    if (!overlaysCaught) {
        bool hasPickers = _imp->viewerTab->getGui()->hasPickers();

        if ( (_imp->pickerState != PICKER_INACTIVE) && buttonDownIsLeft(e) && displayingImage() ) {
            // disable picker if picker is set when clicking
            _imp->pickerState = PICKER_INACTIVE;
            mustRedraw = true;
            overlaysCaught = true;
        } else if ( hasPickers && isMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickColor, modifiers, button) && displayingImage() ) {
            // picker with single-point selection
            _imp->pickerState = PICKER_POINT;
            if ( pickColor( e->x(),e->y() ) ) {
                _imp->ms = PICKING_COLOR;
                mustRedraw = true;
                overlaysCaught = true;
            }
        } else if ( hasPickers && isMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseRectanglePick, modifiers, button) && displayingImage() ) {
            // start picker with rectangle selection (picked color is the average over the rectangle)
            _imp->pickerState = PICKER_RECTANGLE;
            _imp->pickerRect.setTopLeft(zoomPos);
            _imp->pickerRect.setBottomRight(zoomPos);
            _imp->ms = BUILDING_PICKER_RECTANGLE;
            mustRedraw = true;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoIBottomEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the bottom edge of the user ROI
            _imp->ms = DRAGGING_ROI_BOTTOM_EDGE;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoILeftEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the left edge of the user ROI
            _imp->ms = DRAGGING_ROI_LEFT_EDGE;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoIRightEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the right edge of the user ROI
            _imp->ms = DRAGGING_ROI_RIGHT_EDGE;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoITopEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the top edge of the user ROI
            _imp->ms = DRAGGING_ROI_TOP_EDGE;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoI( (userRoI.x1 + userRoI.x2) / 2., (userRoI.y1 + userRoI.y2) / 2.,
                                     zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight ) ) {
            // start dragging the midpoint of the user ROI
            _imp->ms = DRAGGING_ROI_CROSS;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoI(userRoI.x1, userRoI.y2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the topleft corner of the user ROI
            _imp->ms = DRAGGING_ROI_TOP_LEFT;
            overlaysCaught = true;
        } else if ( buttonDownIsLeft(e) &&
                    isNearByUserRoI(userRoI.x2, userRoI.y2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the topright corner of the user ROI
            _imp->ms = DRAGGING_ROI_TOP_RIGHT;
            overlaysCaught = true;
        }  else if ( buttonDownIsLeft(e) &&
                     isNearByUserRoI(userRoI.x1, userRoI.y1, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the bottomleft corner of the user ROI
            _imp->ms = DRAGGING_ROI_BOTTOM_LEFT;
            overlaysCaught = true;
        }  else if ( buttonDownIsLeft(e) &&
                     isNearByUserRoI(userRoI.x2, userRoI.y1, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            // start dragging the bottomright corner of the user ROI
            _imp->ms = DRAGGING_ROI_BOTTOM_RIGHT;
            overlaysCaught = true;
        } else if ( _imp->overlay && isWipeHandleVisible() &&
                    buttonDownIsLeft(e) && _imp->isNearbyWipeCenter(zoomPos, wipeSelectionTol) ) {
            _imp->ms = DRAGGING_WIPE_CENTER;
            overlaysCaught = true;
        } else if ( _imp->overlay &&  isWipeHandleVisible() &&
                    buttonDownIsLeft(e) && _imp->isNearbyWipeMixHandle(zoomPos, wipeSelectionTol) ) {
            _imp->ms = DRAGGING_WIPE_MIX_HANDLE;
            overlaysCaught = true;
        } else if ( _imp->overlay &&  isWipeHandleVisible() &&
                    buttonDownIsLeft(e) && _imp->isNearbyWipeRotateBar(zoomPos, wipeSelectionTol) ) {
            _imp->ms = ROTATING_WIPE_HANDLE;
            overlaysCaught = true;
        }
    }

    if (!overlaysCaught) {
        if ( buttonDownIsRight(e) ) {
            _imp->menu->exec( mapToGlobal( e->pos() ) );
        } else if ( buttonDownIsLeft(e) ) {
            ///build selection rectangle
            _imp->selectionRectangle.setTopLeft(zoomPos);
            _imp->selectionRectangle.setBottomRight(zoomPos);
            _imp->lastDragStartPos = zoomPos;
            _imp->ms = SELECTING;
            if ( !modCASIsControl(e) ) {
                emit selectionCleared();
            }
        }
    }

    if (mustRedraw) {
        updateGL();
    }
} // mousePressEvent

void
ViewerGL::mouseReleaseEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (!_imp->viewerTab->getGui()) {
        return;
    }
    
    bool mustRedraw = false;
    if (_imp->ms == BUILDING_PICKER_RECTANGLE) {
        updateRectangleColorPicker();
    }

    if (_imp->ms == SELECTING) {
        mustRedraw = true;
        emit selectionRectangleChanged(true);
    }

    _imp->ms = UNDEFINED;
    QPointF zoomPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
    }
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    if ( _imp->viewerTab->notifyOverlaysPenUp(1 << mipMapLevel, 1 << mipMapLevel, QMouseEventLocalPos(e), zoomPos, e) ) {
        mustRedraw = true;
    }
    if (mustRedraw) {
        updateGL();
    }
}

void
ViewerGL::mouseMoveEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///The app is closing don't do anything
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }

    QPointF zoomPos;
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();

    // if the picker was deselected, this fixes the picer State
    // (see issue #133 https://github.com/MrKepzie/Natron/issues/133 )
    if ( !_imp->viewerTab->getGui()->hasPickers() ) {
        _imp->pickerState = PICKER_INACTIVE;
    }

    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    Format dispW = getDisplayWindow();
    for (int i = 0; i < 2; ++i) {
        const RectD& rod = getRoD(i);
        updateInfoWidgetColorPicker(zoomPos, e->pos(), width(), height(), rod, dispW, i);
    }
    
    //update the cursor if it is hovering an overlay and we're not dragging the image
    bool userRoIEnabled;
    RectD userRoI;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoI = _imp->userRoI;
        userRoIEnabled = _imp->userRoIEnabled;
    }
    bool mustRedraw = false;
    bool wasHovering = _imp->hs != HOVERING_NOTHING;

    if ( (_imp->ms != DRAGGING_IMAGE) && _imp->overlay ) {
        double wipeSelectionTol;
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            wipeSelectionTol = 8 / _imp->zoomCtx.factor();
        }

        _imp->hs = HOVERING_NOTHING;
        if ( isWipeHandleVisible() && _imp->isNearbyWipeCenter(zoomPos, wipeSelectionTol) ) {
            setCursor( QCursor(Qt::SizeAllCursor) );
        } else if ( isWipeHandleVisible() && _imp->isNearbyWipeMixHandle(zoomPos, wipeSelectionTol) ) {
            _imp->hs = HOVERING_WIPE_MIX;
            mustRedraw = true;
        } else if ( isWipeHandleVisible() && _imp->isNearbyWipeRotateBar(zoomPos, wipeSelectionTol) ) {
            _imp->hs = HOVERING_WIPE_ROTATE_HANDLE;
            mustRedraw = true;
        } else if (userRoIEnabled) {
            if ( isNearByUserRoIBottomEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                 || isNearByUserRoITopEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                 || ( _imp->ms == DRAGGING_ROI_BOTTOM_EDGE)
                 || ( _imp->ms == DRAGGING_ROI_TOP_EDGE) ) {
                setCursor( QCursor(Qt::SizeVerCursor) );
            } else if ( isNearByUserRoILeftEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                        || isNearByUserRoIRightEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                        || ( _imp->ms == DRAGGING_ROI_LEFT_EDGE)
                        || ( _imp->ms == DRAGGING_ROI_RIGHT_EDGE) ) {
                setCursor( QCursor(Qt::SizeHorCursor) );
            } else if ( isNearByUserRoI( (userRoI.x1 + userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight )
                        || ( _imp->ms == DRAGGING_ROI_CROSS) ) {
                setCursor( QCursor(Qt::SizeAllCursor) );
            } else if ( isNearByUserRoI(userRoI.x2, userRoI.y1, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                        isNearByUserRoI(userRoI.x1, userRoI.y2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                        ( _imp->ms == DRAGGING_ROI_BOTTOM_RIGHT) ||
                        ( _imp->ms == DRAGGING_ROI_TOP_LEFT) ) {
                setCursor( QCursor(Qt::SizeFDiagCursor) );
            } else if ( isNearByUserRoI(userRoI.x1, userRoI.y1,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                        isNearByUserRoI(userRoI.x2, userRoI.y2,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                        ( _imp->ms == DRAGGING_ROI_BOTTOM_LEFT) ||
                        ( _imp->ms == DRAGGING_ROI_TOP_RIGHT) ) {
                setCursor( QCursor(Qt::SizeBDiagCursor) );
            } else {
                setCursor( QCursor(Qt::ArrowCursor) );
            }
        } else {
            setCursor( QCursor(Qt::ArrowCursor) );
        }
    } else {
        setCursor( QCursor(Qt::ArrowCursor) );
    }

    if ( (_imp->hs == HOVERING_NOTHING) && wasHovering ) {
        mustRedraw = true;
    }

    QPoint newClick = e->pos();
    QPoint oldClick = _imp->oldClick;
    QPointF newClick_opengl, oldClick_opengl, oldPosition_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        newClick_opengl = _imp->zoomCtx.toZoomCoordinates( newClick.x(),newClick.y() );
        oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( oldClick.x(),oldClick.y() );
        oldPosition_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->lastMousePosition.x(), _imp->lastMousePosition.y() );
    }
    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );
    double dxSinceLastMove = ( oldPosition_opengl.x() - newClick_opengl.x() );
    double dySinceLastMove = ( oldPosition_opengl.y() - newClick_opengl.y() );

    switch (_imp->ms) {
    case DRAGGING_IMAGE: {
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            _imp->zoomCtx.translate(dx, dy);
            _imp->zoomOrPannedSinceLastFit = true;
        }
        _imp->oldClick = newClick;
        if ( displayingImage() ) {
            _imp->viewerTab->getInternalNode()->renderCurrentFrame();
        }
        //  else {
        mustRedraw = true;
        // }
        // no need to update the color picker or mouse posn: they should be unchanged
        break;
    }
    case DRAGGING_ROI_BOTTOM_EDGE: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
            _imp->userRoI.y1 -= dySinceLastMove;
            l.unlock();
            if ( displayingImage() ) {
                _imp->viewerTab->getInternalNode()->renderCurrentFrame();
            }
            mustRedraw = true;
        }
        break;
    }
    case DRAGGING_ROI_LEFT_EDGE: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
            _imp->userRoI.x1 -= dxSinceLastMove;
            l.unlock();
            if ( displayingImage() ) {
                _imp->viewerTab->getInternalNode()->renderCurrentFrame();
            }
            mustRedraw = true;
        }
        break;
    }
    case DRAGGING_ROI_RIGHT_EDGE: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
            _imp->userRoI.x2 -= dxSinceLastMove;
            l.unlock();
            if ( displayingImage() ) {
                _imp->viewerTab->getInternalNode()->renderCurrentFrame();
            }
            mustRedraw = true;
        }
        break;
    }
    case DRAGGING_ROI_TOP_EDGE: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
            _imp->userRoI.y2 -= dySinceLastMove;
            l.unlock();
            if ( displayingImage() ) {
                _imp->viewerTab->getInternalNode()->renderCurrentFrame();
            }
            mustRedraw = true;
        }
        break;
    }
    case DRAGGING_ROI_CROSS: {
        {
            QMutexLocker l(&_imp->userRoIMutex);
            _imp->userRoI.translate(-dxSinceLastMove,-dySinceLastMove);
        }
        if ( displayingImage() ) {
            _imp->viewerTab->getInternalNode()->renderCurrentFrame();
        }
        mustRedraw = true;
        break;
    }
    case DRAGGING_ROI_TOP_LEFT: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
            _imp->userRoI.y2 -= dySinceLastMove;
        }
        if ( (_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
            _imp->userRoI.x1 -= dxSinceLastMove;
        }
        l.unlock();
        if ( displayingImage() ) {
            _imp->viewerTab->getInternalNode()->renderCurrentFrame();
        }
        mustRedraw = true;
        break;
    }
    case DRAGGING_ROI_TOP_RIGHT: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
            _imp->userRoI.y2 -= dySinceLastMove;
        }
        if ( (_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
            _imp->userRoI.x2 -= dxSinceLastMove;
        }
        l.unlock();
        if ( displayingImage() ) {
            _imp->viewerTab->getInternalNode()->renderCurrentFrame();
        }
        mustRedraw = true;
        break;
    }
    case DRAGGING_ROI_BOTTOM_RIGHT: {
        QMutexLocker l(&_imp->userRoIMutex);
        if ( (_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
            _imp->userRoI.x2 -= dxSinceLastMove;
        }
        if ( (_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
            _imp->userRoI.y1 -= dySinceLastMove;
        }
        l.unlock();
        if ( displayingImage() ) {
            _imp->viewerTab->getInternalNode()->renderCurrentFrame();
        }
        mustRedraw = true;
        break;
    }
    case DRAGGING_ROI_BOTTOM_LEFT: {
        if ( (_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
            _imp->userRoI.y1 -= dySinceLastMove;
        }
        if ( (_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
            _imp->userRoI.x1 -= dxSinceLastMove;
        }
        _imp->userRoIMutex.unlock();
        if ( displayingImage() ) {
            _imp->viewerTab->getInternalNode()->renderCurrentFrame();
        }
        mustRedraw = true;
        break;
    }
    case DRAGGING_WIPE_CENTER: {
        QMutexLocker l(&_imp->wipeControlsMutex);
        _imp->wipeCenter.rx() -= dxSinceLastMove;
        _imp->wipeCenter.ry() -= dySinceLastMove;
        mustRedraw = true;
        break;
    }
    case DRAGGING_WIPE_MIX_HANDLE: {
        QMutexLocker l(&_imp->wipeControlsMutex);
        double angle = std::atan2( zoomPos.y() - _imp->wipeCenter.y(), zoomPos.x() - _imp->wipeCenter.x() );
        double prevAngle = std::atan2( oldPosition_opengl.y() - _imp->wipeCenter.y(),
                                       oldPosition_opengl.x() - _imp->wipeCenter.x() );
        _imp->mixAmount -= (angle - prevAngle);
        _imp->mixAmount = std::max( 0.,std::min(_imp->mixAmount,1.) );
        mustRedraw = true;
        break;
    }
    case ROTATING_WIPE_HANDLE: {
        QMutexLocker l(&_imp->wipeControlsMutex);
        double angle = std::atan2( zoomPos.y() - _imp->wipeCenter.y(), zoomPos.x() - _imp->wipeCenter.x() );
        _imp->wipeAngle = angle;
        double mpi2 = M_PI / 2.;
        double closestPI2 = mpi2 * std::floor( (_imp->wipeAngle + M_PI / 4.) / mpi2 );
        if (std::fabs(_imp->wipeAngle - closestPI2) < 0.1) {
            // snap to closest multiple of PI / 2.
            _imp->wipeAngle = closestPI2;
        }

        mustRedraw = true;
        break;
    }
    case PICKING_COLOR: {
        pickColor( newClick.x(), newClick.y() );
        mustRedraw = true;
        break;
    }
    case BUILDING_PICKER_RECTANGLE: {
        QPointF btmRight = _imp->pickerRect.bottomRight();
        btmRight.rx() -= dxSinceLastMove;
        btmRight.ry() -= dySinceLastMove;
        _imp->pickerRect.setBottomRight(btmRight);
        mustRedraw = true;
        break;
    }
    case SELECTING: {
        _imp->refreshSelectionRectangle(zoomPos);
        mustRedraw = true;
        emit selectionRectangleChanged(false);
    }; break;
    default: {
        if ( _imp->overlay &&
             _imp->viewerTab->notifyOverlaysPenMotion(1 << mipMapLevel, 1 << mipMapLevel, QMouseEventLocalPos(e), zoomPos, e) ) {
            mustRedraw = true;
        }
        break;
    }
    } // switch

    if (mustRedraw) {
        updateGL();
    }
    _imp->lastMousePosition = newClick;
    //FIXME: This is bugged, somehow we can't set our custom picker cursor...
//    if(_imp->viewerTab->getGui()->_projectGui->hasPickers()){
//        setCursor(appPTR->getColorPickerCursor());
//    }else{
//        setCursor(QCursor(Qt::ArrowCursor));
//    }
} // mouseMoveEvent

void
ViewerGL::mouseDoubleClickEvent(QMouseEvent* e)
{
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    QPointF pos_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        pos_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(),e->y() );
    }

    if ( _imp->viewerTab->notifyOverlaysPenDoubleClick(1 << mipMapLevel, 1 << mipMapLevel, QMouseEventLocalPos(e), pos_opengl, e) ) {
        updateGL();
    }
    QGLWidget::mouseDoubleClickEvent(e);
}

// used to update the information bar at the bottom of the viewer (not for the ctrl-click color picker)
void
ViewerGL::updateColorPicker(int textureIndex,
                            int x,
                            int y)
{
    if (_imp->pickerState != PICKER_INACTIVE || _imp->viewerTab->getGui()->isGUIFrozen()) {
        return;
    }

    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( !displayingImage() && _imp->infoViewer[textureIndex]->colorAndMouseVisible() ) {
        _imp->infoViewer[textureIndex]->hideColorAndMouseInfo();

        return;
    }

    QPoint pos;
    bool xInitialized = false;
    bool yInitialized = false;
    if (x != INT_MAX) {
        xInitialized = true;
        pos.setX(x);
    }
    if (y != INT_MAX) {
        yInitialized = true;
        pos.setY(y);
    }
    QPoint currentPos = mapFromGlobal( QCursor::pos() );
    if (!xInitialized) {
        pos.setX( currentPos.x() );
    }
    if (!yInitialized) {
        pos.setY( currentPos.y() );
    }
    float r,g,b,a;
    QPointF imgPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPos = _imp->zoomCtx.toZoomCoordinates( pos.x(), pos.y() );
    }
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool picked = false;
    RectD rod = getRoD(textureIndex);
    Format dispW = getDisplayWindow();
    if ( ( imgPos.x() >= rod.left() ) &&
         ( imgPos.x() < rod.right() ) &&
         ( imgPos.y() >= rod.bottom() ) &&
         ( imgPos.y() < rod.top() ) &&
         ( pos.x() >= 0) && ( pos.x() < width() ) &&
         ( pos.y() >= 0) && ( pos.y() < height() ) ) {
        ///if the clip to project format is enabled, make sure it is in the project format too
        bool clipping = isClippingImageToProjectWindow();
        if ( !clipping ||
             ( ( imgPos.x() >= dispW.left() ) &&
               ( imgPos.x() < dispW.right() ) &&
               ( imgPos.y() >= dispW.bottom() ) &&
               ( imgPos.y() < dispW.top() ) ) ) {
            //imgPos must be in canonical coordinates
            picked = getColorAt(imgPos.x(), imgPos.y(), linear, textureIndex, &r, &g, &b, &a);
        }
    }
    if (!picked) {
        if ( _imp->infoViewer[textureIndex]->colorAndMouseVisible() ) {
            _imp->infoViewer[textureIndex]->hideColorAndMouseInfo();
        }
    } else {
        if ( !_imp->infoViewer[textureIndex]->colorAndMouseVisible() ) {
            _imp->infoViewer[textureIndex]->showColorAndMouseInfo();
        }
        _imp->infoViewer[textureIndex]->setColor(r,g,b,a);
    }
} // updateColorPicker

void
ViewerGL::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    _imp->viewerTab->getGui()->selectNode( _imp->viewerTab->getGui()->getApp()->getNodeGui( _imp->viewerTab->getInternalNode()->getNode() ) );

    const double zoomFactor_min = 0.01;
    const double zoomFactor_max = 1024.;
    double zoomFactor;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
        _imp->zoomOrPannedSinceLastFit = true;
    }
    int zoomValue = (int)(100 * zoomFactor);
    if (zoomValue == 0) {
        zoomValue = 1; // sometimes, floor(100*0.01) makes 0
    }
    assert(zoomValue > 0);
    emit zoomChanged(zoomValue);


    if ( displayingImage() ) {
        _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
    }

    ///Clear green cached line so the user doesn't expect to see things in the cache
    ///since we're changing the zoom factor
    _imp->viewerTab->clearTimelineCacheLine();
    updateGL();
}

void
ViewerGL::zoomSlot(int v)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    assert(v > 0);
    double newZoomFactor = v / 100.;
    if (newZoomFactor < 0.01) {
        newZoomFactor = 0.01;
    } else if (newZoomFactor > 1024.) {
        newZoomFactor = 1024.;
    }
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        double scale = newZoomFactor / _imp->zoomCtx.factor();
        double centerX = ( _imp->zoomCtx.left() + _imp->zoomCtx.right() ) / 2.;
        double centerY = ( _imp->zoomCtx.top() + _imp->zoomCtx.bottom() ) / 2.;
        _imp->zoomCtx.zoom(centerX, centerY, scale);
    }
    ///Clear green cached line so the user doesn't expect to see things in the cache
    ///since we're changing the zoom factor
    _imp->viewerTab->clearTimelineCacheLine();
    if ( displayingImage() ) {
        // appPTR->clearPlaybackCache();
        _imp->viewerTab->getInternalNode()->renderCurrentFrame();
    } else {
        updateGL();
    }
}

void
ViewerGL::zoomSlot(QString str)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    str.remove( QChar('%') );
    int v = str.toInt();
    assert(v > 0);
    zoomSlot(v);
}

void
ViewerGL::fitImageToFormat()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    double w,h,zoomPAR;
    const Format & format = _imp->currentViewerInfos[0].getDisplayWindow();
    h = format.height();
    w = format.width();
    zoomPAR = format.getPixelAspect();

    assert(h > 0. && w > 0.);

    double old_zoomFactor;
    double zoomFactor;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        old_zoomFactor = _imp->zoomCtx.factor();
        // set the PAR first
        _imp->zoomCtx.setZoom(0., 0., 1., zoomPAR);
        // leave 4% of margin around
        _imp->zoomCtx.fit(-0.02 * w, 1.02 * w, -0.02 * h, 1.02 * h);
        zoomFactor = _imp->zoomCtx.factor();
        _imp->zoomOrPannedSinceLastFit = false;
    }
    _imp->oldClick = QPoint(); // reset mouse posn

    if (old_zoomFactor != zoomFactor) {
        int zoomFactorInt = zoomFactor  * 100;
        if (zoomFactorInt == 0) {
            zoomFactorInt = 1;
        }
        emit zoomChanged(zoomFactorInt);
    }
    ///Clear green cached line so the user doesn't expect to see things in the cache
    ///since we're changing the zoom factor
    _imp->viewerTab->clearTimelineCacheLine();
}

/**
 *@brief Turns on the overlays on the viewer.
 **/
void
ViewerGL::turnOnOverlay()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->overlay = true;
}

/**
 *@brief Turns off the overlays on the viewer.
 **/
void
ViewerGL::turnOffOverlay()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->overlay = false;
}

void
ViewerGL::setInfoViewer(InfoViewerWidget* i,
                        int textureIndex )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->infoViewer[textureIndex] = i;
}

void
ViewerGL::disconnectViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( displayingImage() ) {
        setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),0);
        setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),1);
    }
    resetWipeControls();
    clearViewer();
}

/* The dataWindow of the currentFrame(BBOX) in canonical coordinates */
const RectD&
ViewerGL::getRoD(int textureIndex) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->currentViewerInfos[textureIndex].getRoD();
}

/*The displayWindow of the currentFrame(Resolution)
   This is the same for both as we only use the display window as to indicate the project window.*/
Format
ViewerGL::getDisplayWindow() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->currentViewerInfos[0].getDisplayWindow();
}

void
ViewerGL::setRegionOfDefinition(const RectD & rod,
                                int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->viewerTab->getGui()) {
        return;
    }
    _imp->currentViewerInfos[textureIndex].setRoD(rod);
    if (_imp->infoViewer[textureIndex] && !_imp->viewerTab->getGui()->isGUIFrozen()) {
        _imp->infoViewer[textureIndex]->setDataWindow(rod);
    }

    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].append( QString::number( std::ceil( rod.left() ) ) );
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].append(",");
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].append( QString::number( std::ceil( rod.bottom() ) ) );
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].append( QString::number( std::floor( rod.right() ) ) );
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].append(",");
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].append( QString::number( std::floor( rod.top() ) ) );
}

void
ViewerGL::onProjectFormatChanged(const Format & format)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if (!_imp->viewerTab->getGui()) {
        return;
    }
    
    _imp->blankViewerInfos.setDisplayWindow(format);
    _imp->blankViewerInfos.setRoD(format);
    for (int i = 0; i < 2; ++i) {
        if (_imp->infoViewer[i]) {
            _imp->infoViewer[i]->setResolution(format);
        }
    }

    _imp->currentViewerInfos[0].setDisplayWindow(format);
    _imp->currentViewerInfos[1].setDisplayWindow(format);
    _imp->currentViewerInfos_resolutionOverlay.clear();
    _imp->currentViewerInfos_resolutionOverlay.append( QString::number( format.width() ) );
    _imp->currentViewerInfos_resolutionOverlay.append("x");
    _imp->currentViewerInfos_resolutionOverlay.append( QString::number( format.height() ) );


    if ( !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject() ) {
        fitImageToFormat();
    }

    if ( displayingImage() ) {
        _imp->viewerTab->getInternalNode()->renderCurrentFrame();
    }

    if (!_imp->isUserRoISet) {
        {
            QMutexLocker l(&_imp->userRoIMutex);
            _imp->userRoI = format;
        }
        _imp->isUserRoISet = true;
    }
}

void
ViewerGL::setClipToDisplayWindow(bool b)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    {
        QMutexLocker l(&_imp->clipToDisplayWindowMutex);
        _imp->clipToDisplayWindow = b;
    }
    ViewerInstance* viewer = _imp->viewerTab->getInternalNode();
    assert(viewer);
    if ( viewer->getUiContext() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject() ) {
        _imp->viewerTab->getInternalNode()->renderCurrentFrame();
    }
}

bool
ViewerGL::isClippingImageToProjectWindow() const
{
    // MT-SAFE
    QMutexLocker l(&_imp->clipToDisplayWindowMutex);

    return _imp->clipToDisplayWindow;
}

/*display black in the viewer*/
void
ViewerGL::clearViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->activeTextures[0] = 0;
    _imp->activeTextures[1] = 0;
    updateGL();
}

/*overload of QT enter/leave/resize events*/
void
ViewerGL::focusInEvent(QFocusEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    ///The app is closing don't do anything
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }
    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if ( _imp->viewerTab->notifyOverlaysFocusGained(scale,scale) ) {
        updateGL();
    }
    QGLWidget::focusInEvent(e);
}

void
ViewerGL::focusOutEvent(QFocusEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !_imp->viewerTab->getGui() ) {
        return;
    }

    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if ( _imp->viewerTab->notifyOverlaysFocusLost(scale,scale) ) {
        updateGL();
    }
    QGLWidget::focusOutEvent(e);
}

void
ViewerGL::enterEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    QWidget* currentFocus = qApp->focusWidget();
    
    bool canSetFocus = !currentFocus ||
    dynamic_cast<ViewerGL*>(currentFocus) ||
    dynamic_cast<CurveWidget*>(currentFocus) ||
    dynamic_cast<Histogram*>(currentFocus) ||
    dynamic_cast<NodeGraph*>(currentFocus) ||
    currentFocus->objectName() == "Properties";
    
    if (canSetFocus) {
        setFocus();
    }
    QWidget::enterEvent(e);
}

void
ViewerGL::leaveEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->infoViewer[0]->hideColorAndMouseInfo();
    _imp->infoViewer[1]->hideColorAndMouseInfo();
    QGLWidget::leaveEvent(e);
}

void
ViewerGL::resizeEvent(QResizeEvent* e)
{ // public to hack the protected field
  // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QGLWidget::resizeEvent(e);
}

void
ViewerGL::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    bool accept = false;

    
    if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideOverlays, modifiers, key) ) {
        toggleOverlays();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideAll, modifiers, key) ) {
        _imp->viewerTab->hideAllToolbars();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionShowAll, modifiers, key) ) {
        _imp->viewerTab->showAllToolbars();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHidePlayer, modifiers, key) ) {
        _imp->viewerTab->togglePlayerVisibility();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideTimeline, modifiers, key) ) {
        _imp->viewerTab->toggleTimelineVisibility();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideInfobar, modifiers, key) ) {
        _imp->viewerTab->toggleInfobarVisbility();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideLeft, modifiers, key) ) {
        _imp->viewerTab->toggleLeftToolbarVisiblity();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideRight, modifiers, key) ) {
        _imp->viewerTab->toggleRightToolbarVisibility();
        accept = true;
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideTop, modifiers, key) ) {
        _imp->viewerTab->toggleTopToolbarVisibility();
        accept = true;
    }

    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if ( e->isAutoRepeat() ) {
        if ( _imp->viewerTab->notifyOverlaysKeyRepeat(scale, scale, e) ) {
            accept = true;
            updateGL();
        }
    } else {
        if ( _imp->viewerTab->notifyOverlaysKeyDown(scale, scale, e) ) {
            accept = true;
            updateGL();
        }
    }
    if (accept) {
        e->accept();
    } else {
        e->ignore();
    }
}

void
ViewerGL::keyReleaseEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->viewerTab->getGui()) {
        return;
    }
    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if ( _imp->viewerTab->notifyOverlaysKeyUp(scale, scale, e) ) {
        updateGL();
    }
}

OpenGLViewerI::BitDepth
ViewerGL::getBitDepth() const
{
    // MT-SAFE
    ///supportsGLSL is set on the main thread only once on startup, it doesn't need to be protected.
    if (!_imp->supportsGLSL) {
        return OpenGLViewerI::BYTE;
    } else {
        // FIXME: casting an int to an enum!

        ///the bitdepth value is locked by the knob holding that value itself.
        return (OpenGLViewerI::BitDepth)appPTR->getCurrentSettings()->getViewersBitDepth();
    }
}

void
ViewerGL::populateMenu()
{
    
    
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->menu->clear();
    QAction* displayOverlaysAction = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideOverlays,kShortcutDescActionHideOverlays, _imp->menu);

    displayOverlaysAction->setCheckable(true);
    displayOverlaysAction->setChecked(true);
    QObject::connect( displayOverlaysAction,SIGNAL( triggered() ),this,SLOT( toggleOverlays() ) );
    
    QMenu* showHideMenu = new QMenu(tr("Show/Hide"),_imp->menu);
    showHideMenu->setFont(QFont(NATRON_FONT,NATRON_FONT_SIZE_11));
    _imp->menu->addAction(showHideMenu->menuAction());
    
    QAction* showHidePlayer,*showHideLeftToolbar,*showHideRightToolbar,*showHideTopToolbar,*showHideInfobar,*showHideTimeline;
    QAction* showAll,*hideAll;
    
    showHidePlayer = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHidePlayer,kShortcutDescActionHidePlayer,
                                                showHideMenu);

    showHideLeftToolbar = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideLeft,kShortcutDescActionHideLeft,
                                                 showHideMenu);
    showHideRightToolbar = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideRight,kShortcutDescActionHideRight,
                                                  showHideMenu);
    showHideTopToolbar = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideTop,kShortcutDescActionHideTop,
                                                showHideMenu);
    showHideInfobar = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideInfobar,kShortcutDescActionHideInfobar,
                                             showHideMenu);
    showHideTimeline = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideTimeline,kShortcutDescActionHideTimeline,
                                              showHideMenu);
   
    
    showAll = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionShowAll,kShortcutDescActionShowAll,
                                     showHideMenu);

    hideAll = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDActionHideAll,kShortcutDescActionHideAll,
                                     showHideMenu);
    
    QObject::connect(showHidePlayer,SIGNAL(triggered()),_imp->viewerTab,SLOT(togglePlayerVisibility()));
    QObject::connect(showHideLeftToolbar,SIGNAL(triggered()),_imp->viewerTab,SLOT(toggleLeftToolbarVisiblity()));
    QObject::connect(showHideRightToolbar,SIGNAL(triggered()),_imp->viewerTab,SLOT(toggleRightToolbarVisibility()));
    QObject::connect(showHideTopToolbar,SIGNAL(triggered()),_imp->viewerTab,SLOT(toggleTopToolbarVisibility()));
    QObject::connect(showHideInfobar,SIGNAL(triggered()),_imp->viewerTab,SLOT(toggleInfobarVisbility()));
    QObject::connect(showHideTimeline,SIGNAL(triggered()),_imp->viewerTab,SLOT(toggleTimelineVisibility()));
    QObject::connect(showAll,SIGNAL(triggered()),_imp->viewerTab,SLOT(showAllToolbars()));
    QObject::connect(hideAll,SIGNAL(triggered()),_imp->viewerTab,SLOT(hideAllToolbars()));
    
    showHideMenu->addAction(showHidePlayer);
    showHideMenu->addAction(showHideTimeline);
    showHideMenu->addAction(showHideInfobar);
    showHideMenu->addAction(showHideLeftToolbar);
    showHideMenu->addAction(showHideRightToolbar);
    showHideMenu->addAction(showHideTopToolbar);
    showHideMenu->addAction(showAll);
    showHideMenu->addAction(hideAll);
    
    _imp->menu->addAction(displayOverlaysAction);
}

void
ViewerGL::renderText(double x,
                     double y,
                     const QString &string,
                     const QColor & color,
                     const QFont & font)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( string.isEmpty() ) {
        return;
    }
    {
        GLProtectAttrib a(GL_TRANSFORM_BIT);
        GLProtectMatrix p(GL_PROJECTION);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double h = (double)height();
        double w = (double)width();
        /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
        glOrtho(0,w,0,h,-1,1);

        QPointF pos;
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            pos = _imp->zoomCtx.toWidgetCoordinates(x, y);
        }
        glCheckError();
        _imp->textRenderer.renderText(pos.x(),h - pos.y(),string,color,font);
        glCheckError();
    } // GLProtectAttrib a(GL_TRANSFORM_BIT);
}

void
ViewerGL::setPersistentMessage(int type,
                               const QString & message)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->persistentMessageType = type;
    _imp->persistentMessage = message;
    _imp->displayPersistentMessage = true;
    updateGL();
}

const QString &
ViewerGL::getCurrentPersistentMessage() const
{
    return _imp->persistentMessage;
}

void
ViewerGL::clearPersistentMessage()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->displayPersistentMessage) {
        return;
    }
    _imp->persistentMessage.clear();
    _imp->displayPersistentMessage = false;
    updateGL();
}

void
ViewerGL::getProjection(double *zoomLeft,
                        double *zoomBottom,
                        double *zoomFactor,
                        double *zoomPAR) const
{
    // MT-SAFE
    QMutexLocker l(&_imp->zoomCtxMutex);

    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomPAR = _imp->zoomCtx.par();
}

void
ViewerGL::setProjection(double zoomLeft,
                        double zoomBottom,
                        double zoomFactor,
                        double zoomPAR)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QMutexLocker l(&_imp->zoomCtxMutex);
    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomPAR);
}

void
ViewerGL::setUserRoIEnabled(bool b)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    {
        QMutexLocker(&_imp->userRoIMutex);
        _imp->userRoIEnabled = b;
    }
    if ( displayingImage() ) {
        _imp->viewerTab->getInternalNode()->renderCurrentFrame();
    }
    update();
}

bool
ViewerGL::isNearByUserRoITopEdge(const RectD & roi,
                                 const QPointF & zoomPos,
                                 double zoomScreenPixelWidth,
                                 double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    double length = std::min(roi.x2 - roi.x1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth) * 2);
    RectD r(roi.x1 + length / 2,
            roi.y2 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight,
            roi.x2 - length / 2,
            roi.y2 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerGL::isNearByUserRoIRightEdge(const RectD & roi,
                                   const QPointF & zoomPos,
                                   double zoomScreenPixelWidth,
                                   double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    double length = std::min(roi.y2 - roi.y1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight) * 2);
    RectD r(roi.x2 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y1 + length / 2,
            roi.x2 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y2 - length / 2);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerGL::isNearByUserRoILeftEdge(const RectD & roi,
                                  const QPointF & zoomPos,
                                  double zoomScreenPixelWidth,
                                  double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    double length = std::min(roi.y2 - roi.y1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight) * 2);
    RectD r(roi.x1 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y1 + length / 2,
            roi.x1 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y2 - length / 2);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerGL::isNearByUserRoIBottomEdge(const RectD & roi,
                                    const QPointF & zoomPos,
                                    double zoomScreenPixelWidth,
                                    double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    double length = std::min(roi.x2 - roi.x1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth) * 2);
    RectD r(roi.x1 + length / 2,
            roi.y1 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight,
            roi.x2 - length / 2,
            roi.y1 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerGL::isNearByUserRoI(double x,
                          double y,
                          const QPointF & zoomPos,
                          double zoomScreenPixelWidth,
                          double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    RectD r(x - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
            y - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
            x + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
            y + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerGL::isUserRegionOfInterestEnabled() const
{
    // MT-SAFE
    QMutexLocker(&_imp->userRoIMutex);

    return _imp->userRoIEnabled;
}

RectD
ViewerGL::getUserRegionOfInterest() const
{
    // MT-SAFE
    QMutexLocker(&_imp->userRoIMutex);

    return _imp->userRoI;
}

void
ViewerGL::setUserRoI(const RectD & r)
{
    // MT-SAFE
    QMutexLocker(&_imp->userRoIMutex);
    _imp->userRoI = r;
}

/**
 * @brief Swap the OpenGL buffers.
 **/
void
ViewerGL::swapOpenGLBuffers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    swapBuffers();
}

/**
 * @brief Repaint
 **/
void
ViewerGL::redraw()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    update();
}

/**
 * @brief Returns the width and height of the viewport in window coordinates.
 **/
void
ViewerGL::getViewportSize(double &width,
                          double &height) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QMutexLocker l(&_imp->zoomCtxMutex);
    width = _imp->zoomCtx.screenWidth();
    height = _imp->zoomCtx.screenHeight();
}

/**
 * @brief Returns the pixel scale of the viewport.
 **/
void
ViewerGL::getPixelScale(double & xScale,
                        double & yScale) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QMutexLocker l(&_imp->zoomCtxMutex);
    xScale = _imp->zoomCtx.screenPixelWidth();
    yScale = _imp->zoomCtx.screenPixelHeight();
}

/**
 * @brief Returns the colour of the background (i.e: clear color) of the viewport.
 **/
void
ViewerGL::getBackgroundColour(double &r,
                              double &g,
                              double &b) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    r = _imp->clearColor.redF();
    g = _imp->clearColor.greenF();
    b = _imp->clearColor.blueF();
}

void
ViewerGL::makeOpenGLcontextCurrent()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();
}

void
ViewerGL::onViewerNodeNameChanged(const QString & name)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->viewerTab->getGui()->unregisterTab(_imp->viewerTab);
    TabWidget* parent = dynamic_cast<TabWidget*>( _imp->viewerTab->parentWidget() );
    if (parent) {
        parent->setTabName(_imp->viewerTab, name);
    }
    _imp->viewerTab->getGui()->registerTab(_imp->viewerTab);
}

void
ViewerGL::removeGUI()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( _imp->viewerTab->getGui() ) {
        _imp->viewerTab->getGui()->removeViewerTab(_imp->viewerTab, true,true);
    }
}

int
ViewerGL::getCurrentView() const
{
    // MT-SAFE

    ///protected in viewerTab (which is const)
    return _imp->viewerTab->getCurrentView();
}

ViewerInstance*
ViewerGL::getInternalNode() const
{
    return _imp->viewerTab->getInternalNode();
}

ViewerTab*
ViewerGL::getViewerTab() const
{
    return _imp->viewerTab;
}

// used for the ctrl-click color picker (not the information bar at the bottom of the viewer)
bool
ViewerGL::pickColor(double x,
                    double y)
{
    float r,g,b,a;
    QPointF imgPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPos = _imp->zoomCtx.toZoomCoordinates(x, y);
    }

    _imp->lastPickerPos = imgPos;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool ret = false;
    for (int i = 0; i < 2; ++i) {
        // imgPos must be in canonical coordinates
        bool picked = getColorAt(imgPos.x(), imgPos.y(), linear, i, &r, &g, &b, &a);
        if (picked) {
            if (i == 0) {
                QColor pickerColor;
                pickerColor.setRedF( Natron::clamp(r) );
                pickerColor.setGreenF( Natron::clamp(g) );
                pickerColor.setBlueF( Natron::clamp(b) );
                pickerColor.setAlphaF( Natron::clamp(a) );
                _imp->viewerTab->getGui()->setColorPickersColor(pickerColor);
            }
            if ( !_imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->showColorAndMouseInfo();
            }
            _imp->infoViewer[i]->setColor(r,g,b,a);
            ret = true;
        } else {
            if ( _imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->hideColorAndMouseInfo();
            }
        }
    }

    return ret;
}

void
ViewerGL::updateInfoWidgetColorPicker(const QPointF & imgPos,
                                      const QPoint & widgetPos,
                                      int width,
                                      int height,
                                      const RectD & rod, // in canonical coordinates
                                      const RectD & dispW, // in canonical coordinates
                                      int texIndex)
{
    if (_imp->viewerTab->getGui()->isGUIFrozen()) {
        return;
    }
    
    if ( _imp->activeTextures[texIndex] &&
         ( imgPos.x() >= rod.left() ) &&
         ( imgPos.x() < rod.right() ) &&
         ( imgPos.y() >= rod.bottom() ) &&
         ( imgPos.y() < rod.top() ) &&
         ( widgetPos.x() >= 0) && ( widgetPos.x() < width) &&
         ( widgetPos.y() >= 0) && ( widgetPos.y() < height) ) {
        ///if the clip to project format is enabled, make sure it is in the project format too
        if ( isClippingImageToProjectWindow() &&
             ( ( imgPos.x() < dispW.left() ) ||
               ( imgPos.x() >= dispW.right() ) ||
               ( imgPos.y() < dispW.bottom() ) ||
               ( imgPos.y() >= dispW.top() ) ) ) {
            if ( _imp->infoViewer[texIndex]->colorAndMouseVisible() ) {
                _imp->infoViewer[texIndex]->hideColorAndMouseInfo();
            }
        } else {
            if (_imp->pickerState == PICKER_INACTIVE) {
                if ( !_imp->viewerTab->getInternalNode()->getRenderEngine()->hasThreadsWorking() ) {
                    updateColorPicker( texIndex,widgetPos.x(),widgetPos.y() );
                }
            } else if ( ( _imp->pickerState == PICKER_POINT) || ( _imp->pickerState == PICKER_RECTANGLE) ) {
                if ( !_imp->infoViewer[texIndex]->colorAndMouseVisible() ) {
                    _imp->infoViewer[texIndex]->showColorAndMouseInfo();
                }
            } else {
                ///unkwn state
                assert(false);
            }

            _imp->infoViewer[texIndex]->setMousePos( QPoint( (int)( imgPos.x() ),(int)( imgPos.y() ) ) );
        }
    } else {
        if ( _imp->infoViewer[texIndex]->colorAndMouseVisible() ) {
            _imp->infoViewer[texIndex]->hideColorAndMouseInfo();
        }
    }
}

void
ViewerGL::updateRectangleColorPicker()
{
    float r,g,b,a;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    QPointF topLeft = _imp->pickerRect.topLeft();
    QPointF btmRight = _imp->pickerRect.bottomRight();
    RectD rect;

    rect.set_left( std::min( topLeft.x(), btmRight.x() ) );
    rect.set_right( std::max( topLeft.x(), btmRight.x() ) );
    rect.set_bottom( std::min( topLeft.y(), btmRight.y() ) );
    rect.set_top( std::max( topLeft.y(), btmRight.y() ) );
    for (int i = 0; i < 2; ++i) {
        bool picked = getColorAtRect(rect, linear, i, &r, &g, &b, &a);
        if (picked) {
            if (i == 0) {
                QColor pickerColor;
                pickerColor.setRedF( clamp(r) );
                pickerColor.setGreenF( clamp(g) );
                pickerColor.setBlueF( clamp(b) );
                pickerColor.setAlphaF( clamp(a) );
                _imp->viewerTab->getGui()->setColorPickersColor(pickerColor);
            }
            if ( !_imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->showColorAndMouseInfo();
            }
            _imp->infoViewer[i]->setColor(r, g, b, a);
        } else {
            if ( _imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->hideColorAndMouseInfo();
            }
        }
    }
}

void
ViewerGL::resetWipeControls()
{
    RectD rod;

    if (_imp->activeTextures[1]) {
        rod = getRoD(1);
    } else if (_imp->activeTextures[0]) {
        rod = getRoD(0);
    } else {
        rod = getDisplayWindow();
    }
    {
        QMutexLocker l(&_imp->wipeControlsMutex);
        _imp->wipeCenter.setX(rod.width() / 2.);
        _imp->wipeCenter.setY(rod.height() / 2.);
        _imp->wipeAngle = 0;
        _imp->mixAmount = 1.;
    }
}

bool
ViewerGL::Implementation::isNearbyWipeCenter(const QPointF & pos,
                                             double tolerance) const
{
    QMutexLocker l(&wipeControlsMutex);

    if ( ( pos.x() >= (wipeCenter.x() - tolerance) ) && ( pos.x() <= (wipeCenter.x() + tolerance) ) &&
         ( pos.y() >= (wipeCenter.y() - tolerance) ) && ( pos.y() <= (wipeCenter.y() + tolerance) ) ) {
        return true;
    }

    return false;
}

bool
ViewerGL::Implementation::isNearbyWipeRotateBar(const QPointF & pos,
                                                double tolerance) const
{
    double rotateLenght,rotateOffset;
    {
        QMutexLocker l(&zoomCtxMutex);
        rotateLenght = WIPE_ROTATE_HANDLE_LENGTH / zoomCtx.factor();
        rotateOffset = WIPE_ROTATE_OFFSET / zoomCtx.factor();
    }
    QMutexLocker l(&wipeControlsMutex);
    QPointF outterPoint;

    outterPoint.setX( wipeCenter.x() + std::cos(wipeAngle) * (rotateLenght - rotateOffset) );
    outterPoint.setY( wipeCenter.y() + std::sin(wipeAngle) * (rotateLenght - rotateOffset) );
    if ( ( ( ( pos.y() >= (wipeCenter.y() - tolerance) ) && ( pos.y() <= (outterPoint.y() + tolerance) ) ) ||
           ( ( pos.y() >= (outterPoint.y() - tolerance) ) && ( pos.y() <= (wipeCenter.y() + tolerance) ) ) ) &&
         ( ( ( pos.x() >= (wipeCenter.x() - tolerance) ) && ( pos.x() <= (outterPoint.x() + tolerance) ) ) ||
           ( ( pos.x() >= (outterPoint.x() - tolerance) ) && ( pos.x() <= (wipeCenter.x() + tolerance) ) ) ) ) {
        Point a;
        a.x = ( outterPoint.x() - wipeCenter.x() );
        a.y = ( outterPoint.y() - wipeCenter.y() );
        double norm = sqrt(a.x * a.x + a.y * a.y);

        ///The point is in the bounding box of the segment, if it is vertical it must be on the segment anyway
        if (norm == 0) {
            return false;
        }

        a.x /= norm;
        a.y /= norm;
        Point b;
        b.x = ( pos.x() - wipeCenter.x() );
        b.y = ( pos.y() - wipeCenter.y() );
        norm = sqrt(b.x * b.x + b.y * b.y);

        ///This vector is not vertical
        if (norm != 0) {
            b.x /= norm;
            b.y /= norm;

            double crossProduct = b.y * a.x - b.x * a.y;
            if (std::abs(crossProduct) <  0.1) {
                return true;
            }
        }
    }

    return false;
}

bool
ViewerGL::Implementation::isNearbyWipeMixHandle(const QPointF & pos,
                                                double tolerance) const
{
    QMutexLocker l(&wipeControlsMutex);
    ///mix 1 is at rotation bar + pi / 8
    ///mix 0 is at rotation bar + 3pi / 8
    double alphaMix1,alphaMix0,alphaCurMix;
    double mpi8 = M_PI / 8;

    alphaMix1 = wipeAngle + mpi8;
    alphaMix0 = wipeAngle + 3. * mpi8;
    alphaCurMix = mixAmount * (alphaMix1 - alphaMix0) + alphaMix0;
    QPointF mixPos;
    double mixLength;
    {
        QMutexLocker l(&zoomCtxMutex);
        mixLength = WIPE_MIX_HANDLE_LENGTH / zoomCtx.factor();
    }

    mixPos.setX(wipeCenter.x() + std::cos(alphaCurMix) * mixLength);
    mixPos.setY(wipeCenter.y() + std::sin(alphaCurMix) * mixLength);
    if ( ( pos.x() >= (mixPos.x() - tolerance) ) && ( pos.x() <= (mixPos.x() + tolerance) ) &&
         ( pos.y() >= (mixPos.y() - tolerance) ) && ( pos.y() <= (mixPos.y() + tolerance) ) ) {
        return true;
    }

    return false;
}

bool
ViewerGL::isWipeHandleVisible() const
{
    return _imp->viewerTab->getCompositingOperator() != OPERATOR_NONE;
}

void
ViewerGL::setZoomOrPannedSinceLastFit(bool enabled)
{
    QMutexLocker l(&_imp->zoomCtxMutex);

    _imp->zoomOrPannedSinceLastFit = enabled;
}

bool
ViewerGL::getZoomOrPannedSinceLastFit() const
{
    QMutexLocker l(&_imp->zoomCtxMutex);

    return _imp->zoomOrPannedSinceLastFit;
}

Natron::ViewerCompositingOperator
ViewerGL::getCompositingOperator() const
{
    return _imp->viewerTab->getCompositingOperator();
}

bool
ViewerGL::isFrameRangeLocked() const
{
    return _imp->viewerTab->isFrameRangeLocked();
}

void
ViewerGL::getTextureColorAt(int x,
                            int y,
                            double* r,
                            double *g,
                            double *b,
                            double *a)
{
    assert( QThread::currentThread() == qApp->thread() );
    makeCurrent();

    *r = 0;
    *g = 0;
    *b = 0;
    *a = 0;

    Texture::DataType type;
    if (_imp->displayTextures[0]) {
        type = _imp->displayTextures[0]->type();
    } else if (_imp->displayTextures[1]) {
        type = _imp->displayTextures[1]->type();
    } else {
        return;
    }

    QPointF pos;
    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        pos = _imp->zoomCtx.toWidgetCoordinates(x, y);
    }

    if ( (type == Texture::BYTE) || !_imp->supportsGLSL ) {
        U32 pixel;
        glReadBuffer(GL_FRONT);
        glReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 red = 0, green = 0, blue = 0, alpha = 0;
        blue |= pixel;
        green |= (pixel >> 8);
        red |= (pixel >> 16);
        alpha |= (pixel >> 24);
        *r = (double)red / 255.;
        *g = (double)green / 255.;
        *b = (double)blue / 255.;
        *a = (double)alpha / 255.;
        glCheckError();
    } else if ( (type == Texture::FLOAT) && _imp->supportsGLSL ) {
        GLfloat pixel[4];
        glReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_RGBA, GL_FLOAT, pixel);
        *r = (double)pixel[0];
        *g = (double)pixel[1];
        *b = (double)pixel[2];
        *a = (double)pixel[3];
        glCheckError();
    }
}

void
ViewerGL::Implementation::refreshSelectionRectangle(const QPointF & pos)
{
    double xmin = std::min( pos.x(),lastDragStartPos.x() );
    double xmax = std::max( pos.x(),lastDragStartPos.x() );
    double ymin = std::min( pos.y(),lastDragStartPos.y() );
    double ymax = std::max( pos.y(),lastDragStartPos.y() );

    selectionRectangle.setRect(xmin,ymin,xmax - xmin,ymax - ymin);
}

void
ViewerGL::getSelectionRectangle(double &left,
                                double &right,
                                double &bottom,
                                double &top) const
{
    QPointF topLeft = _imp->selectionRectangle.topLeft();
    QPointF btmRight = _imp->selectionRectangle.bottomRight();

    left = std::min( topLeft.x(), btmRight.x() );
    right = std::max( topLeft.x(), btmRight.x() );
    bottom = std::min( topLeft.y(), btmRight.y() );
    top = std::max( topLeft.y(), btmRight.y() );
}

boost::shared_ptr<TimeLine>
ViewerGL::getTimeline() const
{
    return _imp->viewerTab->getTimeLine();
}

void
ViewerGL::onCheckerboardSettingsChanged()
{
    _imp->initializeCheckerboardTexture(false);
    update();
}

// used by the RAII class OGLContextSaver
void
ViewerGL::saveOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // set defaults to work around OFX plugin bugs
    glEnable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

// used by the RAII class OGLContextSaver
void
ViewerGL::restoreOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glBindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

void
ViewerGL::clearLastRenderedTexture()
{
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        U64 toUnRegister = 0;
        for (int i = 0; i < 2; ++i) {
            _imp->lastRenderedImage[i].reset();
            toUnRegister += _imp->memoryHeldByLastRenderedImages[i];
        }
        if (toUnRegister > 0) {
            getInternalNode()->unregisterPluginMemory(toUnRegister);
        }
        
    }
}


boost::shared_ptr<Natron::Image>
ViewerGL::getLastRenderedImage(int textureIndex) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if ( !getInternalNode()->getNode()->isActivated() ) {
        return boost::shared_ptr<Natron::Image>();
    }
    QMutexLocker l(&_imp->lastRenderedImageMutex);
    
    return _imp->lastRenderedImage[textureIndex];
}


int
ViewerGL::getMipMapLevelCombinedToZoomFactor() const
{
    int mmLvl = getInternalNode()->getMipMapLevel();
    
    double factor = getZoomFactor();
    if (factor > 1) {
        factor = 1;
    }
    mmLvl = std::max( (double)mmLvl,-std::ceil(std::log(factor) / M_LN2) );
    
    return mmLvl;
}


template <typename PIX,int maxValue>
static
bool
getColorAtInternal(Natron::Image* image,
                   int x,
                   int y,             // in pixel coordinates
                   bool forceLinear,
                   const Natron::Color::Lut* srcColorSpace,
                   const Natron::Color::Lut* dstColorSpace,
                   float* r,
                   float* g,
                   float* b,
                   float* a)
{
    const PIX* pix = (const PIX*)image->pixelAt(x, y);
    
    if (!pix) {
        return false;
    }
    
    Natron::ImageComponents comps = image->getComponents();
    switch (comps) {
        case Natron::ImageComponentRGBA:
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = pix[2] / (float)maxValue;
            *a = pix[3] / (float)maxValue;
            break;
        case Natron::ImageComponentRGB:
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = pix[2] / (float)maxValue;
            *a = 1.;
            break;
        case Natron::ImageComponentAlpha:
            *r = 0.;
            *g = 0.;
            *b = 0.;
            *a = pix[3] / (float)maxValue;
            break;
        default:
            assert(false);
            break;
    }
    
    
    ///convert to linear
    if (srcColorSpace) {
        *r = srcColorSpace->fromColorSpaceFloatToLinearFloat(*r);
        *g = srcColorSpace->fromColorSpaceFloatToLinearFloat(*g);
        *b = srcColorSpace->fromColorSpaceFloatToLinearFloat(*b);
    }
    
    if (!forceLinear && dstColorSpace) {
        ///convert to dst color space
        float from[3];
        from[0] = *r;
        from[1] = *g;
        from[2] = *b;
        float to[3];
        dstColorSpace->to_float_planar(to, from, 3);
        *r = to[0];
        *g = to[1];
        *b = to[2];
    }
    
    return true;
} // getColorAtInternal

bool
ViewerGL::getColorAt(double x,
                           double y,           // x and y in canonical coordinates
                           bool forceLinear,
                           int textureIndex,
                           float* r,
                           float* g,
                           float* b,
                           float* a)                               // output values
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);
    
    boost::shared_ptr<Image> img;
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        img = _imp->lastRenderedImage[textureIndex];
    }

    unsigned int mipMapLevel = (unsigned int)getMipMapLevelCombinedToZoomFactor();
    if ( !img || (img->getMipMapLevel() != mipMapLevel) ) {
        double colorGPU[4];
        getTextureColorAt(x, y, &colorGPU[0], &colorGPU[1], &colorGPU[2], &colorGPU[3]);
        *a = colorGPU[3];
        if ( forceLinear && (_imp->displayingImageLut != Linear) ) {
            const Natron::Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
            
            *r = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[0]);
            *g = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[1]);
            *b = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[2]);
        } else {
            *r = colorGPU[0];
            *g = colorGPU[1];
            *b = colorGPU[2];
        }
        
        return true;
    }
    
    Natron::ImageBitDepth depth = img->getBitDepth();
    ViewerColorSpace srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Natron::Color::Lut* dstColorSpace;
    const Natron::Color::Lut* srcColorSpace;
    if ( (srcCS == _imp->displayingImageLut) && ( (_imp->displayingImageLut == Linear) || !forceLinear ) ) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        srcColorSpace = ViewerInstance::lutFromColorspace(srcCS);
        dstColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
    }
    
    ///Convert to pixel coords
    int xPixel = int( std::floor(x) ) >> mipMapLevel;
    int yPixel = int( std::floor(y) ) >> mipMapLevel;
    bool gotval;
    switch (depth) {
        case IMAGE_BYTE:
            gotval = getColorAtInternal<unsigned char, 255>(img.get(),
                                                            xPixel, yPixel,
                                                            forceLinear,
                                                            srcColorSpace,
                                                            dstColorSpace,
                                                            r, g, b, a);
            break;
        case IMAGE_SHORT:
            gotval = getColorAtInternal<unsigned short, 65535>(img.get(),
                                                               xPixel, yPixel,
                                                               forceLinear,
                                                               srcColorSpace,
                                                               dstColorSpace,
                                                               r, g, b, a);
            break;
        case IMAGE_FLOAT:
            gotval = getColorAtInternal<float, 1>(img.get(),
                                                  xPixel, yPixel,
                                                  forceLinear,
                                                  srcColorSpace,
                                                  dstColorSpace,
                                                  r, g, b, a);
            break;
        default:
            gotval = false;
            break;
    }
    
    return gotval;
} // getColorAt

bool
ViewerGL::getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                               bool forceLinear,
                               int textureIndex,
                               float* r,
                               float* g,
                               float* b,
                               float* a)                               // output values
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);
    
    boost::shared_ptr<Image> img;
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        img = _imp->lastRenderedImage[textureIndex];
    }
  
    unsigned int mipMapLevel = (unsigned int)getMipMapLevelCombinedToZoomFactor();
    ///Convert to pixel coords
    RectI rectPixel;
    rectPixel.set_left(  int( std::floor( rect.left() ) ) >> mipMapLevel);
    rectPixel.set_right( int( std::floor( rect.right() ) ) >> mipMapLevel);
    rectPixel.set_bottom(int( std::floor( rect.bottom() ) ) >> mipMapLevel);
    rectPixel.set_top(   int( std::floor( rect.top() ) ) >> mipMapLevel);
    assert( rect.bottom() <= rect.top() && rect.left() <= rect.right() );
    assert( rectPixel.bottom() <= rectPixel.top() && rectPixel.left() <= rectPixel.right() );
    double rSum = 0.;
    double gSum = 0.;
    double bSum = 0.;
    double aSum = 0.;
    if ( !img || (img->getMipMapLevel() != mipMapLevel) ) {
        double colorGPU[4];
        for (int yPixel = rectPixel.bottom(); yPixel < rectPixel.top(); ++yPixel) {
            for (int xPixel = rectPixel.left(); xPixel < rectPixel.right(); ++xPixel) {
                getTextureColorAt(xPixel << mipMapLevel, yPixel << mipMapLevel,
                                                   &colorGPU[0], &colorGPU[1], &colorGPU[2], &colorGPU[3]);
                aSum += colorGPU[3];
                if ( forceLinear && (_imp->displayingImageLut != Linear) ) {
                    const Natron::Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
                    
                    rSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[0]);
                    gSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[1]);
                    bSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[2]);
                } else {
                    rSum += colorGPU[0];
                    gSum += colorGPU[1];
                    bSum += colorGPU[2];
                }
            }
        }
        *r = rSum / rectPixel.area();
        *g = gSum / rectPixel.area();
        *b = bSum / rectPixel.area();
        *a = aSum / rectPixel.area();
        
        return true;
    }
    
    
    Natron::ImageBitDepth depth = img->getBitDepth();
    ViewerColorSpace srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Natron::Color::Lut* dstColorSpace;
    const Natron::Color::Lut* srcColorSpace;
    if ( (srcCS == _imp->displayingImageLut) && ( (_imp->displayingImageLut == Linear) || !forceLinear ) ) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        srcColorSpace = ViewerInstance::lutFromColorspace(srcCS);
        dstColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
    }
    
    unsigned long area = 0;
    for (int yPixel = rectPixel.bottom(); yPixel < rectPixel.top(); ++yPixel) {
        for (int xPixel = rectPixel.left(); xPixel < rectPixel.right(); ++xPixel) {
            float rPix, gPix, bPix, aPix;
            bool gotval = false;
            switch (depth) {
                case IMAGE_BYTE:
                    gotval = getColorAtInternal<unsigned char, 255>(img.get(),
                                                                    xPixel, yPixel,
                                                                    forceLinear,
                                                                    srcColorSpace,
                                                                    dstColorSpace,
                                                                    &rPix, &gPix, &bPix, &aPix);
                    break;
                case IMAGE_SHORT:
                    gotval = getColorAtInternal<unsigned short, 65535>(img.get(),
                                                                       xPixel, yPixel,
                                                                       forceLinear,
                                                                       srcColorSpace,
                                                                       dstColorSpace,
                                                                       &rPix, &gPix, &bPix, &aPix);
                    break;
                case IMAGE_FLOAT:
                    gotval = getColorAtInternal<float, 1>(img.get(),
                                                          xPixel, yPixel,
                                                          forceLinear,
                                                          srcColorSpace,
                                                          dstColorSpace,
                                                          &rPix, &gPix, &bPix, &aPix);
                    break;
                case IMAGE_NONE:
                    break;
            }
            if (gotval) {
                rSum += rPix;
                gSum += gPix;
                bSum += bPix;
                aSum += aPix;
                ++area;
            }
        }
    }
    
    if (area > 0) {
        *r = rSum / area;
        *g = gSum / area;
        *b = bSum / area;
        *a = aSum / area;
        
        return true;
    }
    
    return false;
} // getColorAtRect

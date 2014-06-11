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
#include "Gui/ZoomContext.h"

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
    PICKING_COLOR,
    BUILDING_PICKER_RECTANGLE,
    DRAGGING_WIPE_CENTER,
    DRAGGING_WIPE_MIX_HANDLE,
    ROTATING_WIPE_HANDLE,
    UNDEFINED};
    
enum HOVER_STATE{
    HOVERING_NOTHING = 0,
    HOVERING_WIPE_MIX,
    HOVERING_WIPE_ROTATE_HANDLE
    
};
} // namespace

enum PickerState {
    PICKER_INACTIVE = 0,
    PICKER_POINT,
    PICKER_RECTANGLE
};

struct ViewerGL::Implementation {

    Implementation(ViewerTab* parent, ViewerGL* _this)
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
    , displayingImageLut(Natron::sRGB)
    , ms(UNDEFINED)
    , hs(HOVERING_NOTHING)
    , textRenderingColor(200,200,200,255)
    , displayWindowOverlayColor(125,125,125,255)
    , rodOverlayColor(100,100,100,255)
    , textFont(new QFont(NATRON_FONT, NATRON_FONT_SIZE_13))
    , overlay(true)
    , supportsGLSL(true)
    , updatingTexture(false)
    , clearColor(0,0,0,255)
    , menu(new QMenu(_this))
    , persistentMessage()
    , persistentMessageType(0)
    , displayPersistentMessage(false)
    , textRenderer()
    , isUserRoISet(false)
    , lastMousePosition()
    , currentViewerInfos()
    , currentViewerInfos_btmLeftBBOXoverlay()
    , currentViewerInfos_topRightBBOXoverlay()
    , currentViewerInfos_resolutionOverlay()
    , pickerState(PICKER_INACTIVE)
    , userRoIEnabled(false) // protected by mutex
    , userRoI() // protected by mutex
    , zoomCtx() // protected by mutex
    , clipToDisplayWindow(true) // protected by mutex
    , wipeControlsMutex()
    , mixAmount(1.) // protected by mutex
    , wipeAngle(M_PI / 2.) // protected by mutex
    , wipeCenter()
    {
        infoViewer[0] = 0;
        infoViewer[1] = 0;
        displayTextures[0] = 0;
        displayTextures[1] = 0;
        activeTextures[0] = 0;
        activeTextures[1] = 0;
        displayingImageGain[0] = displayingImageGain[1] = 1.;
        displayingImageOffset[0] = displayingImageOffset[1] = 0.;
        assert(qApp && qApp->thread() == QThread::currentThread());
    }

    /////////////////////////////////////////////////////////
    // The following are only accessed from the main thread:

    std::vector<GLuint> pboIds; //!< PBO's id's used by the OpenGL context
    //   GLuint vaoId; //!< VAO holding the rendering VBOs for texture mapping.
    GLuint vboVerticesId; //!< VBO holding the vertices for the texture mapping.
    GLuint vboTexturesId; //!< VBO holding texture coordinates.
    GLuint iboTriangleStripId; /*!< IBOs holding vertices indexes for triangle strip sets*/
    
    Texture* activeTextures[2];/*!< A pointer to the current textures used to display. One for A and B. May point to blackTex */
    Texture* displayTextures[2];/*!< A pointer to the textures that would be used if A and B are displayed*/
    QGLShaderProgram* shaderRGB;/*!< The shader program used to render RGB data*/
    QGLShaderProgram* shaderBlack;/*!< The shader program used when the viewer is disconnected.*/
    bool shaderLoaded;/*!< Flag to check whether the shaders have already been loaded.*/
    InfoViewerWidget* infoViewer[2];/*!< Pointer to the info bar below the viewer holding pixel/mouse/format related infos*/
    ViewerTab* const viewerTab;/*!< Pointer to the viewer tab GUI*/
    bool zoomOrPannedSinceLastFit; //< true if the user zoomed or panned the image since the last call to fitToRoD
    QPoint oldClick;
    ImageInfo blankViewerInfos;/*!< Pointer to the infos used when the viewer is disconnected.*/

    double displayingImageGain[2];
    double displayingImageOffset[2];
    unsigned int displayingImageMipMapLevel;
    Natron::ViewerColorSpace displayingImageLut;

    MOUSE_STATE ms;/*!< Holds the mouse state*/
    HOVER_STATE hs;

    const QColor textRenderingColor;
    const QColor displayWindowOverlayColor;
    const QColor rodOverlayColor;
    QFont* textFont;

    bool overlay;/*!< True if the user enabled overlay dispay*/

    // supportsGLSL is accessed from several threads, but is set only once at startup
    bool supportsGLSL;/*!< True if the user has a GLSL version supporting everything requested.*/

    bool updatingTexture;

    QColor clearColor;
    
    QMenu* menu;

    QString persistentMessage;
    int persistentMessageType;
    bool displayPersistentMessage;

    Natron::TextRenderer textRenderer;

    bool isUserRoISet;
    QPoint lastMousePosition;
    
    /////// currentViewerInfos
    ImageInfo currentViewerInfos[2];/*!< Pointer to the ViewerInfos  used for rendering*/
    QString currentViewerInfos_btmLeftBBOXoverlay[2];/*!< The string holding the bottom left corner coordinates of the dataWindow*/
    QString currentViewerInfos_topRightBBOXoverlay[2];/*!< The string holding the top right corner coordinates of the dataWindow*/
    QString currentViewerInfos_resolutionOverlay;/*!< The string holding the resolution overlay, e.g: "1920x1080"*/

    ///////Picker infos, used only by the main-thread
    PickerState pickerState;
    QPointF lastPickerPos;
    QRectF pickerRect;
    
    //////////////////////////////////////////////////////////
    // The following are accessed from various threads
    QMutex userRoIMutex;
    bool userRoIEnabled;
    RectI userRoI; //< in canonical coords

    ZoomContext zoomCtx;/*!< All zoom related variables are packed into this object. */
    mutable QMutex zoomCtxMutex; /// protectx zoomCtx*

    QMutex clipToDisplayWindowMutex;
    bool clipToDisplayWindow;
 
    
    mutable QMutex wipeControlsMutex;
    double mixAmount; /// the amount of the second input to blend, by default 1.
    double wipeAngle; /// the angle to the X axis
    QPointF wipeCenter; /// the center of the wipe control
    
    
    bool isNearbyWipeCenter(const QPointF& pos,double tolerance) const;
    bool isNearbyWipeRotateBar(const QPointF& pos,double tolerance) const;
    bool isNearbyWipeMixHandle(const QPointF& pos,double tolerance) const;
    
    void drawArcOfCircle(const QPointF& center,double radius,double startAngle,double endAngle);
    
    void bindTextureAndActivateShader(int i,bool useShader)
    {
        assert(activeTextures[i]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, activeTextures[i]->getTexID());
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
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    /**
     *@brief Starts using the RGB shader to display the frame
     **/
    void activateShaderRGB(int texIndex);
};

#if 0
/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int
toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;

unsigned int
toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
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
void ViewerGL::drawRenderingVAO(unsigned int mipMapLevel,int textureIndex,bool drawOnlyWipePolygon,ViewerCompositingOperator compOp)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    
    ///the texture rectangle in image coordinates. The values in it are multiples of tile size.
    ///
    const TextureRect &r = _imp->activeTextures[textureIndex]->getTextureRect();
    
    ///This is the coordinates in the image being rendered where datas are valid, this is in pixel coordinates
    ///at the time we initialize it but we will convert it later to canonical coordinates. See 1)
    RectI texRect(r.x1,r.y1,r.x2,r.y2);
    
    
    ///the RoD of the iamge in canonical coords.
    RectI rod = getRoD(textureIndex);
    bool clipToDisplayWindow;
    {
        QMutexLocker l(&_imp->clipToDisplayWindowMutex);
        clipToDisplayWindow = _imp->clipToDisplayWindow;
    }
    if (clipToDisplayWindow) {
        ///clip the RoD to the project format.
        if (!rod.intersect(getDisplayWindow(),&rod)) {
            return;
        }
    }
    
    ///texRectCLipped is at full res always
    RectI texRectClipped = texRect;
    ///If proxy is enabled
    if (mipMapLevel != 0) {
        
        ///1) convert the image data rectangle to canonical coords
        texRect = texRect.upscalePowerOfTwo(mipMapLevel);
        
        ///texRect was created using the downscaleSmallestEnclosingPot function, so upscaling it by a pot will require a clip to the RoD.
        texRect.intersect(rod, &texRectClipped);
        
    }
    
    //if user RoI is enabled, clip the rod to that roi
    bool userRoiEnabled;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoiEnabled = _imp->userRoIEnabled;
    }
    if (userRoiEnabled) {
        {
            QMutexLocker l(&_imp->userRoIMutex);
            //if the userRoI isn't intersecting the rod, just don't render anything
            if (!rod.intersect(_imp->userRoI,&rod)) {
                return;
            }
        }
        texRectClipped.intersect(rod, &texRectClipped);
    }
    
    ////The texture real size (r.w,r.h) might be slightly bigger than the actual
    ////pixel coordinates bounds r.x1,r.x2 r.y1 r.y2 because we clipped these bounds against the pixelRoD
    ////in the ViewerInstance::renderViewer function. That means we need to draw actually only the part of
    ////the texture that contains the bounds.
    ////Notice that r.w and r.h are scaled to the closest Po2 of the current scaling factor, so we need to scale it up
    ////So it is in the same coordinates as the bounds.
    GLfloat texBottom,texLeft,texRight,texTop;
    texBottom =  0;
    texTop =  (GLfloat)(r.y2 - r.y1)  / (GLfloat)(r.h * r.closestPo2);
    texLeft = 0;
    texRight = (GLfloat)(r.x2 - r.x1)  / (GLfloat)(r.w * r.closestPo2);

    
    assert((texRect.y2  - texRect.y1) > 0 &&(texRect.x2  - texRect.x1) > 0 &&
           (texRectClipped.y1 - texRect.y1) <= (texRect.y2 - texRect.y1) &&
           (texRectClipped.x1 - texRect.x1) <= (texRect.x2 - texRect.x1) &&
           texRectClipped.y2 <= texRect.y2 &&
           texRectClipped.x2 <= texRect.x2);

    
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    QPointF wipeCenter;
    double wipeAngle;
    double wipeMix;
    {
        QMutexLocker l(&_imp->wipeControlsMutex);
        wipeCenter = _imp->wipeCenter;
        wipeAngle = _imp->wipeAngle;
        wipeMix = _imp->mixAmount;
    }
    glBlendColor(1, 1, 1, wipeMix);
#pragma message WARN("There might be bugs left here I did it fast")
    if (compOp == OPERATOR_WIPE) {
        glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
    } else if (compOp == OPERATOR_OVER) {
        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA);
    } else if (compOp == OPERATOR_UNDER) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (compOp == OPERATOR_MINUS) {
        glBlendFunc(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    }

    
    if (drawOnlyWipePolygon) {
        /// draw only  the plane defined by the wipe handle
    
        ///Compute a second point on the plane separator line
        ///we don't really care how far it is from the center point, it just has to be on the line
        QPointF firstPoint,secondPoint;
        double mpi2 = M_PI / 2.;
        
        ///extrapolate the line to the maximum size of the RoD so we're sure the line
        ///intersection algorithm works
        double maxSize = std::max(rod.x2 - rod.x1,rod.y2 - rod.y1) * 100000.;
        firstPoint.setX(wipeCenter.x() - std::cos(wipeAngle + mpi2) * maxSize);
        firstPoint.setY(wipeCenter.y() - std::sin(wipeAngle + mpi2) * maxSize);
        secondPoint.setX(wipeCenter.x() + std::cos(wipeAngle + mpi2) * maxSize);
        secondPoint.setY(wipeCenter.y() + std::sin(wipeAngle + mpi2) * maxSize);
        
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
        
        assert(numIntersec == 0 || numIntersec == 2);
        
        ///determine the orientation of the planes
        double crossProd  = (secondPoint.x() - wipeCenter.x()) * (texRectClipped.y1 - wipeCenter.y())
        - (secondPoint.y() - wipeCenter.y()) * (texRectClipped.x1 - wipeCenter.x());
        if (numIntersec == 0) {
            
            ///the bottom left corner is on the left plane
            if (crossProd > 0 && (wipeCenter.x() >= texRectClipped.x2 || wipeCenter.y() >= texRectClipped.y2)) {
                ///the plane is invisible because the wipe handle is below or on the left of the texRectClipped
                glDisable(GL_BLEND);
                glPopAttrib();
                return;
            }
            ///otherwise we draw the entire texture as usual
            drawOnlyWipePolygon = false;
        } else {
            ///we have 2 intersects
            assert(validIntersectionsIndex[0] != -1 && validIntersectionsIndex[1] != -1);
            QPolygonF polygonPoints;
            bool isBottomLeftOnLeftPlane = crossProd > 0;
            
            ///there are 6 cases
            if (crossingBtm && crossingLeft) {
                if (isBottomLeftOnLeftPlane) {
                    //btm intersect is the first
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x1,texRectClipped.y2));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x2,texRectClipped.y2));
                    polygonPoints.insert(4 ,QPointF(texRectClipped.x2,texRectClipped.y1));
                    polygonPoints.insert(5 ,intersections[validIntersectionsIndex[0]]);
                } else {
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x1,texRectClipped.y1));
                    polygonPoints.insert(3 ,intersections[validIntersectionsIndex[0]]);
                }
                
            } else if (crossingBtm && crossingTop) {
                if (isBottomLeftOnLeftPlane) {
                    ///btm intersect is the second
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y2));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x2,texRectClipped.y1));
                    polygonPoints.insert(4 ,intersections[validIntersectionsIndex[1]]);
                    
                } else {
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x1,texRectClipped.y2));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x1,texRectClipped.y1));
                    polygonPoints.insert(4 ,intersections[validIntersectionsIndex[1]]);
                }
            } else if (crossingBtm && crossingRight) {
                if (isBottomLeftOnLeftPlane) {
                    ///btm intersect is the second
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y1));
                    polygonPoints.insert(3 ,intersections[validIntersectionsIndex[1]]);
                } else {
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y2));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x1,texRectClipped.y2));
                    polygonPoints.insert(4 ,QPointF(texRectClipped.x1,texRectClipped.y1));
                    polygonPoints.insert(5 ,intersections[validIntersectionsIndex[1]]);
                }
                
            } else if (crossingLeft && crossingTop) {
                if (isBottomLeftOnLeftPlane) {
                    ///left intersect is the second
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x1,texRectClipped.y2));
                    polygonPoints.insert(3 ,intersections[validIntersectionsIndex[1]]);
                } else {
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y2));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x2,texRectClipped.y1));
                    polygonPoints.insert(4 ,QPointF(texRectClipped.x1,texRectClipped.y1));
                    polygonPoints.insert(5 ,intersections[validIntersectionsIndex[1]]);
                }
            } else if (crossingLeft && crossingRight) {
                if (isBottomLeftOnLeftPlane) {
                    ///left intersect is the second
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,QPointF(texRectClipped.x1,texRectClipped.y2));
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y2));
                    polygonPoints.insert(3 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(4 ,intersections[validIntersectionsIndex[1]]);
                    
                } else {
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y1));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x1,texRectClipped.y1));
                    polygonPoints.insert(4 ,intersections[validIntersectionsIndex[1]]);
                }
            } else if (crossingTop && crossingRight) {
                if (isBottomLeftOnLeftPlane) {
                    ///right is second
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(1 ,QPointF(texRectClipped.x2,texRectClipped.y2));
                    polygonPoints.insert(2 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(3 ,intersections[validIntersectionsIndex[0]]);
                } else {
                    polygonPoints.insert(0 ,intersections[validIntersectionsIndex[0]]);
                    polygonPoints.insert(1 ,intersections[validIntersectionsIndex[1]]);
                    polygonPoints.insert(2 ,QPointF(texRectClipped.x2,texRectClipped.y1));
                    polygonPoints.insert(3 ,QPointF(texRectClipped.x1,texRectClipped.y1));
                    polygonPoints.insert(4 ,QPointF(texRectClipped.x1,texRectClipped.y2));
                    polygonPoints.insert(5 ,intersections[validIntersectionsIndex[0]]);
                }
            } else {
                assert(false);
            }
            
            QPolygonF polygonTexCoords(polygonPoints.size());
            for (int i = 0; i < polygonPoints.size(); ++i) {
                const QPointF& polygonPoint = polygonPoints.at(i);
                QPointF texCoord;
                texCoord.setX((polygonPoint.x() - texRect.x1) / (texRect.x2 - texRect.x1) * (texRight - texLeft));
                texCoord.setY((polygonPoint.y() - texRect.y1) / (texRect.y2 - texRect.y1) * (texTop - texBottom));
                polygonTexCoords[i] = texCoord;
            }
            
            
            
            glBegin(GL_POLYGON);
            for (int i = 0; i < polygonTexCoords.size(); ++i) {
                const QPointF& tCoord = polygonTexCoords[i];
                const QPointF& vCoord = polygonPoints[i];
                glTexCoord2d(tCoord.x(), tCoord.y());
                glVertex2d(vCoord.x(), vCoord.y());
            }
            glEnd();

           
        }
    }
    
    if (!drawOnlyWipePolygon) {
        ///Vertices are in canonical coords
        GLfloat vertices[32] = {
            (GLfloat)rod.left() ,(GLfloat)rod.top()  , //0
            (GLfloat)texRectClipped.x1       , (GLfloat)rod.top()  , //1
            (GLfloat)texRectClipped.x2 , (GLfloat)rod.top()  , //2
            (GLfloat)rod.right(),(GLfloat)rod.top()  , //3
            (GLfloat)rod.left(), (GLfloat)texRectClipped.y2, //4
            (GLfloat)texRectClipped.x1      ,  (GLfloat)texRectClipped.y2, //5
            (GLfloat)texRectClipped.x2,  (GLfloat)texRectClipped.y2, //6
            (GLfloat)rod.right(),(GLfloat)texRectClipped.y2, //7
            (GLfloat)rod.left() ,(GLfloat)texRectClipped.y1      , //8
            (GLfloat)texRectClipped.x1      ,  (GLfloat)texRectClipped.y1      , //9
            (GLfloat)texRectClipped.x2,  (GLfloat)texRectClipped.y1      , //10
            (GLfloat)rod.right(),(GLfloat)texRectClipped.y1      , //11
            (GLfloat)rod.left(), (GLfloat)rod.bottom(), //12
            (GLfloat)texRectClipped.x1      ,  (GLfloat)rod.bottom(), //13
            (GLfloat)texRectClipped.x2,  (GLfloat)rod.bottom(), //14
            (GLfloat)rod.right(),(GLfloat)rod.bottom() //15
        };
        
        
        
        ///Now  the texture coordinates must be adjusted according to the clipping we applied.
        GLfloat texBottomTmp,texLeftTmp,texRightTmp,texTopTmp;
        
        texBottomTmp = (GLfloat)(texRectClipped.y1 - texRect.y1) / (GLfloat) (texRect.y2 - texRect.y1) * (texTop - texBottom);
        texTopTmp = (GLfloat)(texRectClipped.y2 - texRect.y1) / (GLfloat) (texRect.y2  - texRect.y1) * (texTop - texBottom);
        texLeftTmp = (GLfloat)(texRectClipped.x1 - texRect.x1) / (GLfloat) (texRect.x2 - texRect.x1) * (texRight - texLeft);
        texRightTmp = (GLfloat)(texRectClipped.x2  - texRect.x1) / (GLfloat) (texRect.x2  - texRect.x1) * (texRight - texLeft);
        texBottom = texBottomTmp;
        texTop = texTopTmp;
        texLeft = texLeftTmp;
        texRight = texRightTmp;
        
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
        
        
        
        glCheckError();
        
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
        glCheckErrorIgnoreOSXBug();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glCheckError();
    }
    glDisable(GL_BLEND);
    glPopAttrib();
}

#if 0
void ViewerGL::checkFrameBufferCompleteness(const char where[],bool silent)
{
    // always running in the main thread
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
    glCheckError();
}
#endif

ViewerGL::ViewerGL(ViewerTab* parent,const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _imp(new Implementation(parent, this))
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    //setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    QObject::connect(parent->getGui()->getApp()->getProject().get(),SIGNAL(formatChanged(Format)),this,SLOT(onProjectFormatChanged(Format)));
    
    populateMenu();

    
    _imp->blankViewerInfos.setChannels(Natron::Mask_RGBA);

    Format projectFormat;
    parent->getGui()->getApp()->getProject()->getProjectDefaultFormat(&projectFormat);

    _imp->blankViewerInfos.setRoD(projectFormat);
    _imp->blankViewerInfos.setDisplayWindow(projectFormat);
    setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),0);
    setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),1);
    resetWipeControls();
    onProjectFormatChanged(projectFormat);
    
    QObject::connect(getInternalNode(), SIGNAL(rodChanged(RectI,int)), this, SLOT(setRegionOfDefinition(RectI,int)));

}


ViewerGL::~ViewerGL()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    makeCurrent();
    
    if(_imp->shaderRGB){
        _imp->shaderRGB->removeAllShaders();
        delete _imp->shaderRGB;
    }
    if(_imp->shaderBlack){
        _imp->shaderBlack->removeAllShaders();
        delete _imp->shaderBlack;
        
    }
    delete _imp->displayTextures[0]; 
    delete _imp->displayTextures[1];
    glCheckError();
    for(U32 i = 0; i < _imp->pboIds.size();++i){
        glDeleteBuffers(1,&_imp->pboIds[i]);
    }
    glCheckError();
    glDeleteBuffers(1, &_imp->vboVerticesId);
    glDeleteBuffers(1, &_imp->vboTexturesId);
    glDeleteBuffers(1, &_imp->iboTriangleStripId);
    glCheckError();
    delete _imp->textFont;
}

QSize ViewerGL::sizeHint() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    return QSize(1920,1080);
}

const QFont& ViewerGL::textFont() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    return *_imp->textFont;
}

void ViewerGL::setTextFont(const QFont& f)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    *_imp->textFont = f;
}


/**
 *@returns Returns true if the viewer is displaying something.
 **/
bool ViewerGL::displayingImage() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->activeTextures[0] != 0 || _imp->activeTextures[1] != 0;
}

void ViewerGL::resizeGL(int width, int height)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if(height == 0 || width == 0) {// prevent division by 0
        return;
    }
    glViewport (0, 0, width, height);
    bool zoomSinceLastFit;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
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
    if (viewer->getUiContext() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject()) {
        viewer->refreshAndContinueRender(false);
        updateGL();
    }
}

void ViewerGL::paintGL()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    glCheckError();
   
    glMatrixMode (GL_PROJECTION);
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
    if (zoomLeft == zoomRight || zoomTop == zoomBottom) {
        clearColorBuffer(_imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF());
        return;
    }

    glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, -1, 1);
    glCheckError();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    clearColorBuffer(_imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF());
    glCheckErrorIgnoreOSXBug();
    
    glEnable (GL_TEXTURE_2D);
    
    // don't even bind the shader on 8-bits gamma-compressed textures
    bool useShader = getBitDepth() != OpenGLViewerI::BYTE && _imp->supportsGLSL;
    
    ViewerCompositingOperator compOp = _imp->viewerTab->getCompositingOperator();
    
    bool dontDrawTexture[2];
    dontDrawTexture[0] = !_imp->activeTextures[0];
    dontDrawTexture[1] = !_imp->activeTextures[1] ||  ///the texture is null
    (_imp->activeTextures[0] == _imp->activeTextures[1] ) //or it is the input B and it is equal to the input A
    || (compOp != OPERATOR_NONE && !_imp->activeTextures[0]) //or it is input B and comp is not NONE and there is no input A
    || (_imp->activeTextures[0] && _imp->activeTextures[1] && compOp == OPERATOR_NONE); //or it is input B and input A  and B
                                                                                                  //are valid but comp OP is NONE.
    
    if (compOp != OPERATOR_OVER) {
        ///In wipe mode draw first the input A then only the portion we are interested in the input B
        for (int i = 0; i < 2 ; ++i) {
            
            if (dontDrawTexture[i]) { continue; }
            
            _imp->bindTextureAndActivateShader(i, useShader);
            drawRenderingVAO(_imp->displayingImageMipMapLevel,i, i == 1,compOp);
            _imp->unbindTextureAndReleaseShader(useShader);
            
        }

    } else if (compOp == OPERATOR_OVER) {
        ///draw first B then A then just the portion we're interested in of B
        for (int i = 1; i >= 0 ; --i) {
            
           if (dontDrawTexture[i]) { continue; }
            
            _imp->bindTextureAndActivateShader(i, useShader);
            drawRenderingVAO(_imp->displayingImageMipMapLevel,i, i == 1,compOp);
            _imp->unbindTextureAndReleaseShader(useShader);
        }
        if (!dontDrawTexture[1]) {
            _imp->bindTextureAndActivateShader(1, useShader);
            drawRenderingVAO(_imp->displayingImageMipMapLevel,1, true,OPERATOR_WIPE);
            _imp->unbindTextureAndReleaseShader(useShader);
        }
    } else

    glCheckError();
    if (_imp->overlay) {
        drawOverlay(_imp->displayingImageMipMapLevel);
    }

    if (_imp->displayPersistentMessage) {
        drawPersistentMessage();
    }
    glCheckErrorAssert();
}


void ViewerGL::clearColorBuffer(double r ,double g ,double b ,double a )
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ViewerGL::toggleOverlays()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->overlay = !_imp->overlay;
    updateGL();
}


void ViewerGL::drawOverlay(unsigned int mipMapLevel)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    glCheckError();
    RectI dispW = getDisplayWindow();
    
    renderText(dispW.right(),dispW.bottom(), _imp->currentViewerInfos_resolutionOverlay,_imp->textRenderingColor,*_imp->textFont);

    
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
    glCheckErrorIgnoreOSXBug();
    
    
    for (int i = 0 ; i< 2 ;++i) {
        if (!_imp->activeTextures[i]) {
            break;
        }
        RectI dataW = getRoD(i);
        
        if(dataW != dispW){
            
            
            renderText(dataW.right(), dataW.top(),
                       _imp->currentViewerInfos_topRightBBOXoverlay[i], _imp->rodOverlayColor,*_imp->textFont);
            renderText(dataW.left(), dataW.bottom(),
                       _imp->currentViewerInfos_btmLeftBBOXoverlay[i], _imp->rodOverlayColor,*_imp->textFont);
            
            
            
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
    
    _imp->viewerTab->drawOverlays(1 << mipMapLevel,1 << mipMapLevel);

    if (_imp->pickerState == PICKER_RECTANGLE) {
        drawPickerRectangle();
    } else if (_imp->pickerState == PICKER_POINT) {
        drawPickerPixel();
    }
    
    
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
    glCheckError();
}

void ViewerGL::drawUserRoI()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    glColor4f(0.9, 0.9, 0.9, 1.);

    double zoomScreenPixelWidth, zoomScreenPixelHeight;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }

    RectI userRoI;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoI = _imp->userRoI;
    }

    ///base rect
    glBegin(GL_LINE_STRIP);
    glVertex2f(userRoI.x1, userRoI.y1); //bottom left
    glVertex2f(userRoI.x1, userRoI.y2); //top left
    glVertex2f(userRoI.x2, userRoI.y2); //top right
    glVertex2f(userRoI.x2, userRoI.y1); //bottom right
    glVertex2f(userRoI.x1, userRoI.y1); //bottom left
    glEnd();
    
    
    glBegin(GL_LINES);
    ///border ticks
    double borderTickWidth = USER_ROI_BORDER_TICK_SIZE * zoomScreenPixelWidth;
    double borderTickHeight = USER_ROI_BORDER_TICK_SIZE * zoomScreenPixelHeight;
    glVertex2f(userRoI.x1, (userRoI.y1 + userRoI.y2) / 2);
    glVertex2f(userRoI.x1 - borderTickWidth, (userRoI.y1 + userRoI.y2) / 2);
    
    glVertex2f(userRoI.x2, (userRoI.y1 + userRoI.y2) / 2);
    glVertex2f(userRoI.x2 + borderTickWidth, (userRoI.y1 + userRoI.y2) / 2);
    
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2, userRoI.y2);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2, userRoI.y2 + borderTickHeight);
    
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2, userRoI.y1);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2, userRoI.y1 - borderTickHeight);
    
    ///middle cross
    double crossWidth = USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth;
    double crossHeight = USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight;
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2 - crossHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2 + crossHeight);
    
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2  - crossWidth, (userRoI.y1 + userRoI.y2) / 2);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2  + crossWidth, (userRoI.y1 + userRoI.y2) / 2);
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
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y2 - rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y2 + rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y2 + rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y2 - rectHalfHeight);

    //right
    glVertex2f(userRoI.x2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
    glVertex2f(userRoI.x2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
    glVertex2f(userRoI.x2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
    glVertex2f(userRoI.x2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
    
    //bottom
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y1 - rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y1 + rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y1 + rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y1 - rectHalfHeight);

    //middle
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
    glVertex2f((userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
    
    
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
}

void ViewerGL::drawWipeControl()
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
    rotateAxisRight.setX(wipeCenter.x() + std::cos(wipeAngle) * (rotateLenght - rotateOffset));
    rotateAxisRight.setY(wipeCenter.y() + std::sin(wipeAngle) * (rotateLenght - rotateOffset));
    rotateAxisLeft.setX(wipeCenter.x() - std::cos(wipeAngle) * rotateOffset);
    rotateAxisLeft.setY(wipeCenter.y() - (std::sin(wipeAngle) * rotateOffset));
    
    oppositeAxisTop.setX(wipeCenter.x() + std::cos(wipeAngle + M_PI / 2.) * (rotateLenght / 2.));
    oppositeAxisTop.setY(wipeCenter.y() + std::sin(wipeAngle + M_PI / 2.) * (rotateLenght / 2.));
    oppositeAxisBottom.setX(wipeCenter.x() - std::cos(wipeAngle + M_PI / 2.) * (rotateLenght / 2.));
    oppositeAxisBottom.setY(wipeCenter.y() - std::sin(wipeAngle + M_PI / 2.) * (rotateLenght / 2.));
    
    glLineWidth(2);
    glBegin(GL_LINES);
    if (_imp->hs == HOVERING_WIPE_ROTATE_HANDLE || _imp->ms == ROTATING_WIPE_HANDLE) {
        glColor4f(0., 1., 0., 1.);
    } else {
        glColor4f(0.8, 0.8, 0.8, 1.);
    }
    glVertex2d(rotateAxisLeft.x(), rotateAxisLeft.y());
    glVertex2d(rotateAxisRight.x(), rotateAxisRight.y());
    glColor4f(0.8, 0.8, 0.8, 1.);
    glVertex2d(oppositeAxisBottom.x(), oppositeAxisBottom.y());
    glVertex2d(oppositeAxisTop.x(), oppositeAxisTop.y());
    glVertex2d(wipeCenter.x(),wipeCenter.y());
    glVertex2d(mixPos.x(), mixPos.y());
    glEnd();
    glLineWidth(1.);

    ///if hovering the rotate handle or dragging it show a small bended arrow
    if (_imp->hs == HOVERING_WIPE_ROTATE_HANDLE || _imp->ms == ROTATING_WIPE_HANDLE) {
        
        glColor4f(0., 1., 0., 1.);
        double arrowCenterX = WIPE_ROTATE_HANDLE_LENGTH / (2. * zoomFactor);
        ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
        OfxPointD arrowRadius;
        arrowRadius.x = 5. / zoomFactor;
        arrowRadius.y = 10. / zoomFactor;
        
        glPushMatrix ();
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
        
        glPopMatrix ();
    }
    
    glPointSize(5.);
    glBegin(GL_POINTS);
    glVertex2d(wipeCenter.x(), wipeCenter.y());
    if ((_imp->hs == HOVERING_WIPE_MIX && _imp->ms != ROTATING_WIPE_HANDLE) || _imp->ms == DRAGGING_WIPE_MIX_HANDLE) {
        glColor4f(0., 1., 0., 1.);
    } else {
        glColor4f(0.8, 0.8, 0.8, 1.);
    }
    glVertex2d(mixPos.x(), mixPos.y());
    glEnd();
    glPointSize(1.);
    
    _imp->drawArcOfCircle(wipeCenter, mixLength, wipeAngle + M_PI / 8., wipeAngle + 3. * M_PI / 8.);
    
}

void ViewerGL::Implementation::drawArcOfCircle(const QPointF& center,double radius,double startAngle,double endAngle)
{
    double alpha = startAngle;
    double x,y;
    if (hs == HOVERING_WIPE_MIX || ms == DRAGGING_WIPE_MIX_HANDLE) {
        glColor3f(0, 1, 0);
    } else {
        glColor3f(0.8, 0.8, 0.8);
    }
    glBegin(GL_POINTS);
    while (alpha <= endAngle) {
        x = center.x()  + radius * std::cos(alpha);
        y = center.y()  + radius * std::sin(alpha);
        glVertex2d(x, y);
        alpha += 0.01;
    }
    glEnd();
}

void ViewerGL::drawPickerRectangle()
{
    glColor3f(0.9, 0.7, 0.);
    QPointF topLeft = _imp->pickerRect.topLeft();
    QPointF btmRight = _imp->pickerRect.bottomRight();
    ///base rect
    glBegin(GL_LINE_STRIP);
    glVertex2f(topLeft.x(), btmRight.y()); //bottom left
    glVertex2f(topLeft.x(), topLeft.y()); //top left
    glVertex2f(btmRight.x(), topLeft.y()); //top right
    glVertex2f(btmRight.x(), btmRight.y()); //bottom right
    glVertex2f(topLeft.x(), btmRight.y()); //bottom left
    glEnd();
}

void ViewerGL::drawPickerPixel()
{
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        glPointSize(1. * _imp->zoomCtx.factor());
    }
    
    QPointF pos = _imp->lastPickerPos;
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    if (mipMapLevel != 0) {
        pos *= (1 << mipMapLevel);
        
    }
    
    glEnable(GL_POINT_SMOOTH);
    glColor3f(0.9, 0.7, 0.);
    glBegin(GL_POINTS);
    glVertex2d(pos.x(),pos.y());
    glEnd();
    glPointSize(1.);
    glDisable(GL_POINT_SMOOTH);
}

void ViewerGL::drawPersistentMessage()
{
    // always running in the main thread
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
        if (i < _imp->persistentMessage.size() && _imp->persistentMessage.at(i) == QChar(' ')) {
            ++i;
        }
        lines.append(str);
    }
    
    int offset = metrics.height()+10;
    QPointF topLeft, bottomRight, textPos;
    double zoomScreenPixelHeight;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        topLeft = _imp->zoomCtx.toZoomCoordinates(0,0);
        bottomRight = _imp->zoomCtx.toZoomCoordinates(_imp->zoomCtx.screenWidth(),numberOfLines*(metrics.height()*2));
        textPos = _imp->zoomCtx.toZoomCoordinates(20, offset);
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    if (_imp->persistentMessageType == 1) { // error
        glColor4f(0.5,0.,0.,1.);
    } else { // warning
        glColor4f(0.65,0.65,0.,1.);
    }
    glBegin(GL_POLYGON);
    glVertex2f(topLeft.x(),topLeft.y()); //top left
    glVertex2f(topLeft.x(),bottomRight.y()); //bottom left
    glVertex2f(bottomRight.x(),bottomRight.y());//bottom right
    glVertex2f(bottomRight.x(),topLeft.y()); //top right
    glEnd();
    
    

    
    for (int j = 0 ; j < lines.size();++j) {
        renderText(textPos.x(),textPos.y(), lines.at(j),_imp->textRenderingColor,*_imp->textFont);
        textPos.setY(textPos.y() - metrics.height()*2 * zoomScreenPixelHeight);
    }
    glCheckError();
    //reseting color for next pass
    glColor4f(1., 1., 1., 1.);
}



void ViewerGL::initializeGL()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
	makeCurrent();
    initAndCheckGlExtensions();
    _imp->displayTextures[0] = new Texture(GL_TEXTURE_2D,GL_LINEAR,GL_NEAREST);
    _imp->displayTextures[1] = new Texture(GL_TEXTURE_2D,GL_LINEAR,GL_NEAREST);
    
    
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
    glCheckError();
    
    
    if(_imp->supportsGLSL){
        initShaderGLSL();
        glCheckError();
    }
    
    glCheckError();
}

QString ViewerGL::getOpenGLVersionString() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    const char* str = (const char*)glGetString(GL_VERSION);
    QString ret;
    if (str) {
        ret.append(str);
    }
    return ret;
}

QString ViewerGL::getGlewVersionString() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    const char* str = reinterpret_cast<const char *>(glewGetString(GLEW_VERSION));
    QString ret;
    if (str) {
        ret.append(str);
    }
    return ret;
}

GLuint ViewerGL::getPboID(int index)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    if(index >= (int)_imp->pboIds.size()){
        GLuint handle;
        glGenBuffers(1,&handle);
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
    // MT-SAFE
    QMutexLocker l(&_imp->zoomCtxMutex);
    return _imp->zoomCtx.factor();
}

RectI ViewerGL::getImageRectangleDisplayed(const RectI& imageRoD)
{
    // MT-SAFE
    RectI ret;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF topLeft =  _imp->zoomCtx.toZoomCoordinates(0, 0);
        ret.x1 = std::floor(topLeft.x());
        ret.y2 = std::ceil(topLeft.y());
        QPointF bottomRight = _imp->zoomCtx.toZoomCoordinates(width()-1, height()-1);
        ret.x2 = std::ceil(bottomRight.x());
        ret.y1 = std::floor(bottomRight.y());
    }
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    
    if (mipMapLevel != 0) {
        // for the viewer, we need the smallest enclosing rectangle at the mipmap level, in order to avoid black borders
        ret = ret.downscalePowerOfTwoSmallestEnclosing(mipMapLevel);
    }
    
    ///If the roi doesn't intersect the image's Region of Definition just return an empty rectangle
    if (!ret.intersect(imageRoD, &ret)) {
        ret.clear();
    }
    
    
    {
        QMutexLocker l(&_imp->userRoIMutex);
        if (_imp->userRoIEnabled) {
            RectI userRoI = _imp->userRoI;
            if (mipMapLevel != 0) {
                ///If the user roi is enabled, we want to render the smallest enclosing rectangle in order to avoid black borders.
                userRoI = userRoI.downscalePowerOfTwoSmallestEnclosing(mipMapLevel);
            }
            
            ///If the user roi doesn't intersect the actually visible portion on the viewer, return an empty rectangle.
            if (!ret.intersect(userRoI, &ret)) {
                ret.clear();
            }
        }
    }
    return ret;
}



int ViewerGL::isExtensionSupported(const char *extension)
{
    // always running in the main thread
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
    // always running in the main thread
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
    
    _imp->viewerTab->getGui()->setOpenGLVersion(getOpenGLVersionString());
    _imp->viewerTab->getGui()->setGlewVersion(getGlewVersionString());
    
    if (!QGLShaderProgram::hasOpenGLShaderPrograms(context())) {
        // no need to pull out a dialog, it was already presented after the GLEW check above

        //Natron::errorDialog("Viewer error","The viewer is unabgile to work without a proper version of GLSL.");
        //cout << "Warning : GLSL not present on this hardware, no material acceleration possible." << endl;
        _imp->supportsGLSL = false;
    }
}


void ViewerGL::Implementation::activateShaderRGB(int texIndex)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    // we assume that:
    // - 8-bits textures are stored non-linear and must be displayer as is
    // - floating-point textures are linear and must be decompressed according to the given lut

    assert(supportsGLSL );
    
    if (!shaderRGB->bind()) {
        cout << qPrintable(shaderRGB->log()) << endl;
    }
    
    shaderRGB->setUniformValue("Tex", 0);
    shaderRGB->setUniformValue("gain", (float)displayingImageGain[texIndex]);
    shaderRGB->setUniformValue("offset", (float)displayingImageOffset[texIndex]);
    shaderRGB->setUniformValue("lut", (GLint)displayingImageLut);

    
}

void ViewerGL::initShaderGLSL()
{
    // always running in the main thread
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
    // always running in the main thread
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
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}




void ViewerGL::transferBufferFromRAMtoGPU(const unsigned char* ramBuffer, size_t bytesCount, const TextureRect& region, double gain, double offset, int lut, int pboIndex,unsigned int mipMapLevel,int textureIndex)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());
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
    glCheckError();
    assert(ret);
    
    memcpy(ret, (void*)ramBuffer, bytesCount);

    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glCheckError();
    
    OpenGLViewerI::BitDepth bd = getBitDepth();
    assert(textureIndex == 0 || textureIndex == 1);
    if (bd == OpenGLViewerI::BYTE) {
        _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region,Texture::BYTE);
    } else if(bd == OpenGLViewerI::FLOAT || bd == OpenGLViewerI::HALF_FLOAT) {
        //do 32bit fp textures either way, don't bother with half float. We might support it further on.
        _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region,Texture::FLOAT);
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
    _imp->activeTextures[textureIndex] = _imp->displayTextures[textureIndex];
    _imp->displayingImageGain[textureIndex] = gain;
    _imp->displayingImageOffset[textureIndex] = offset;
    _imp->displayingImageMipMapLevel = mipMapLevel;
    _imp->displayingImageLut = (Natron::ViewerColorSpace)lut;

    emit imageChanged(textureIndex);
}

void ViewerGL::disconnectInputTexture(int textureIndex)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(textureIndex == 0 || textureIndex == 1);
    _imp->activeTextures[textureIndex] = 0;
    
}

void ViewerGL::setGain(double d)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->displayingImageGain[0] = d;
    _imp->displayingImageGain[1] = d;
}

void ViewerGL::setLut(int lut)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->displayingImageLut = (Natron::ViewerColorSpace)lut;
}

/**
 *@returns Returns true if the graphic card supports GLSL.
 **/
bool ViewerGL::supportsGLSL() const
{
    // always running in the main thread
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
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (event->button() == Qt::RightButton) {
        _imp->menu->exec(mapToGlobal(event->pos()));
        return;
    } else if (event->button() == Qt::LeftButton) {
        _imp->viewerTab->getGui()->selectNode(_imp->viewerTab->getGui()->getApp()->getNodeGui(_imp->viewerTab->getInternalNode()->getNode()));
    }

    _imp->oldClick = event->pos();
    _imp->lastMousePosition = event->pos();
    QPointF zoomPos;
    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates(event->x(), event->y());
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    RectI rod = getRoD(0);

    RectI userRoI;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoI = _imp->userRoI;
    }

    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _imp->ms = DRAGGING_IMAGE;
        return;
    }
    
    bool mouseInDispW = rod.contains(zoomPos.x(), zoomPos.y());
    
    bool overlaysCaught = false;
    
    
    double wipeSelectionTol;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        wipeSelectionTol = 8. / _imp->zoomCtx.factor();
    }
    
    if (_imp->overlay && isWipeHandleVisible() &&
        event->button() == Qt::LeftButton && _imp->isNearbyWipeCenter(zoomPos, wipeSelectionTol)) {
        _imp->ms = DRAGGING_WIPE_CENTER;
        return;
    } else if (_imp->overlay &&  isWipeHandleVisible() &&
               event->button() == Qt::LeftButton && _imp->isNearbyWipeMixHandle(zoomPos, wipeSelectionTol)) {
        _imp->ms = DRAGGING_WIPE_MIX_HANDLE;
        return;
    } else if (_imp->overlay &&  isWipeHandleVisible() &&
               event->button() == Qt::LeftButton && _imp->isNearbyWipeRotateBar(zoomPos, wipeSelectionTol)) {
        _imp->ms = ROTATING_WIPE_HANDLE;
        return;
    }
    
    if (event->button() == Qt::LeftButton && _imp->ms == UNDEFINED && _imp->overlay) {
        unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
        overlaysCaught = _imp->viewerTab->notifyOverlaysPenDown(1 << mipMapLevel,1 << mipMapLevel,QMouseEventLocalPos(event),zoomPos);
        if (overlaysCaught) {
            updateGL();
            return;
        }
    }
    
    if (event->button() == Qt::LeftButton &&
        !event->modifiers().testFlag(Qt::ControlModifier) && !event->modifiers().testFlag(Qt::ShiftModifier) &&
        displayingImage()) {
        _imp->pickerState = PICKER_INACTIVE;
        updateGL();
    }
    
    if(event->button() == Qt::LeftButton &&
              event->modifiers().testFlag(Qt::ControlModifier) && !event->modifiers().testFlag(Qt::ShiftModifier) &&
              displayingImage() && mouseInDispW) {
        _imp->pickerState = PICKER_POINT;
        if (pickColor(event->x(),event->y())) {
            _imp->ms = PICKING_COLOR;
            updateGL();
        }
    } else if (event->button() == Qt::LeftButton &&
               event->modifiers().testFlag(Qt::ControlModifier) && event->modifiers().testFlag(Qt::ShiftModifier) &&
               displayingImage() && mouseInDispW) {
        _imp->pickerState = PICKER_RECTANGLE;
        _imp->pickerRect.setTopLeft(zoomPos);
        _imp->pickerRect.setBottomRight(zoomPos);
        _imp->ms = BUILDING_PICKER_RECTANGLE;
        updateGL(); 
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoIBottomEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_BOTTOM_EDGE;
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoILeftEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_LEFT_EDGE;
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoIRightEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_RIGHT_EDGE;
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoITopEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_TOP_EDGE;
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoIMiddleHandle(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_CROSS;
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoITopLeft(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_TOP_LEFT;
    } else if(event->button() == Qt::LeftButton &&
              isNearByUserRoITopRight(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_TOP_RIGHT;
    }  else if(event->button() == Qt::LeftButton &&
               isNearByUserRoIBottomLeft(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_BOTTOM_LEFT;
    }  else if(event->button() == Qt::LeftButton &&
               isNearByUserRoIBottomRight(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)) {
        _imp->ms = DRAGGING_ROI_BOTTOM_RIGHT;
    }
   
    
}

void ViewerGL::mouseReleaseEvent(QMouseEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    
    if (_imp->ms == BUILDING_PICKER_RECTANGLE) {
        updateRectangleColorPicker();
    }
    
    _imp->ms = UNDEFINED;
    QPointF zoomPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates(event->x(), event->y());
    }
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    if (_imp->viewerTab->notifyOverlaysPenUp(1 << mipMapLevel, 1 << mipMapLevel,QMouseEventLocalPos(event), zoomPos)) {
        updateGL();
    }

}
void ViewerGL::mouseMoveEvent(QMouseEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    QPointF zoomPos;
    
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();

    
    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates(event->x(), event->y());
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    Format dispW = getDisplayWindow();
    for (int i = 0; i< 2; ++i) {
        RectI rod = getRoD(i);
        updateInfoWidgetColorPicker(zoomPos, event->pos(),width(),height(),rod,dispW,i);
    }
    //update the cursor if it is hovering an overlay and we're not dragging the image
    bool userRoIEnabled;
    RectI userRoI;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoI = _imp->userRoI;
        userRoIEnabled = _imp->userRoIEnabled;
    }

    bool mustRedraw = false;
    bool wasHovering = _imp->hs != HOVERING_NOTHING;
    
    if (_imp->ms != DRAGGING_IMAGE && _imp->overlay) {
        
        double wipeSelectionTol;
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            wipeSelectionTol = 8 / _imp->zoomCtx.factor();
        }
        
        _imp->hs = HOVERING_NOTHING;
        if (isWipeHandleVisible() && _imp->isNearbyWipeCenter(zoomPos, wipeSelectionTol)) {
            setCursor(QCursor(Qt::SizeAllCursor));
        } else if (isWipeHandleVisible() && _imp->isNearbyWipeMixHandle(zoomPos, wipeSelectionTol)) {
            _imp->hs = HOVERING_WIPE_MIX;
            mustRedraw = true;
        } else if (isWipeHandleVisible() && _imp->isNearbyWipeRotateBar(zoomPos, wipeSelectionTol)) {
            _imp->hs = HOVERING_WIPE_ROTATE_HANDLE;
            mustRedraw = true;
        } else if (userRoIEnabled) {
            if (isNearByUserRoIBottomEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                || isNearByUserRoITopEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                || _imp->ms == DRAGGING_ROI_BOTTOM_EDGE
                || _imp->ms == DRAGGING_ROI_TOP_EDGE) {
                setCursor(QCursor(Qt::SizeVerCursor));
            } else if(isNearByUserRoILeftEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || isNearByUserRoIRightEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || _imp->ms == DRAGGING_ROI_LEFT_EDGE
                      || _imp->ms == DRAGGING_ROI_RIGHT_EDGE) {
                setCursor(QCursor(Qt::SizeHorCursor));
            } else if(isNearByUserRoIMiddleHandle(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || _imp->ms == DRAGGING_ROI_CROSS) {
                setCursor(QCursor(Qt::SizeAllCursor));
            } else if(isNearByUserRoIBottomRight(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || isNearByUserRoITopLeft(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || _imp->ms == DRAGGING_ROI_BOTTOM_RIGHT) {
                setCursor(QCursor(Qt::SizeFDiagCursor));
                
            } else if(isNearByUserRoIBottomLeft(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || isNearByUserRoITopRight(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                      || _imp->ms == DRAGGING_ROI_BOTTOM_LEFT) {
                setCursor(QCursor(Qt::SizeBDiagCursor));
                
            } else {
                setCursor(QCursor(Qt::ArrowCursor));
            }
        } else {
            setCursor(QCursor(Qt::ArrowCursor));
        }
    } else {
        setCursor(QCursor(Qt::ArrowCursor));
    }

    if (_imp->hs == HOVERING_NOTHING && wasHovering) {
        mustRedraw = true;
    }
    
    QPoint newClick = event->pos();
    QPoint oldClick = _imp->oldClick;

    QPointF newClick_opengl, oldClick_opengl, oldPosition_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        newClick_opengl = _imp->zoomCtx.toZoomCoordinates(newClick.x(),newClick.y());
        oldClick_opengl = _imp->zoomCtx.toZoomCoordinates(oldClick.x(),oldClick.y());
        oldPosition_opengl = _imp->zoomCtx.toZoomCoordinates(_imp->lastMousePosition.x(), _imp->lastMousePosition.y());
    }

    double dx = (oldClick_opengl.x() - newClick_opengl.x());
    double dy = (oldClick_opengl.y() - newClick_opengl.y());
    double dxSinceLastMove = (oldPosition_opengl.x() - newClick_opengl.x());
    double dySinceLastMove = (oldPosition_opengl.y() - newClick_opengl.y());

    switch (_imp->ms) {
        case DRAGGING_IMAGE: {
            {
                QMutexLocker l(&_imp->zoomCtxMutex);
                _imp->zoomCtx.translate(dx, dy);
                _imp->zoomOrPannedSinceLastFit = true;
            }
            _imp->oldClick = newClick;
            if(displayingImage()){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
            }
            //  else {
            mustRedraw = true;
            // }
            // no need to update the color picker or mouse posn: they should be unchanged
        } break;
        case DRAGGING_ROI_BOTTOM_EDGE: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
                l.unlock();
                if(displayingImage()){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
                }
                mustRedraw = true;
            }
        } break;
        case DRAGGING_ROI_LEFT_EDGE: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
                l.unlock();
                if(displayingImage()){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
                }
                mustRedraw = true;
            }
        } break;
        case DRAGGING_ROI_RIGHT_EDGE: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
                l.unlock();
                if(displayingImage()){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
                }
                mustRedraw = true;
            }
        } break;
        case DRAGGING_ROI_TOP_EDGE: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
                l.unlock();
                if(displayingImage()){
                    _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
                }
                mustRedraw = true;
            }
        } break;
        case DRAGGING_ROI_CROSS: {
            {
                QMutexLocker l(&_imp->userRoIMutex);
                _imp->userRoI.move(-dxSinceLastMove,-dySinceLastMove);
            }
            if(displayingImage()){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
            }
            mustRedraw = true;
        } break;
        case DRAGGING_ROI_TOP_LEFT: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
            }
            if ((_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
            }
            l.unlock();
            if(displayingImage()){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
            }
            mustRedraw = true;
        } break;
        case DRAGGING_ROI_TOP_RIGHT: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
            }
            if ((_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
            }
            l.unlock();
            if(displayingImage()){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
            }
            mustRedraw = true;
        } break;
        case DRAGGING_ROI_BOTTOM_RIGHT: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ((_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
            }
            if ((_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
            }
            l.unlock();
            if(displayingImage()){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
            }
            mustRedraw = true;
        } break;
        case DRAGGING_ROI_BOTTOM_LEFT: {
            if ((_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
            }
            if ((_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
            }
            _imp->userRoIMutex.unlock();
            if(displayingImage()){
                _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
            }
            mustRedraw = true;
        } break;
        case DRAGGING_WIPE_CENTER: {
            QMutexLocker l(&_imp->wipeControlsMutex);
            _imp->wipeCenter.rx() -= dxSinceLastMove;
            _imp->wipeCenter.ry() -= dySinceLastMove;
            mustRedraw = true;
        }   break;
        case DRAGGING_WIPE_MIX_HANDLE: {
            QMutexLocker l(&_imp->wipeControlsMutex);
            
            double angle = std::atan2(zoomPos.y() - _imp->wipeCenter.y() , zoomPos.x() - _imp->wipeCenter.x());
            double prevAngle = std::atan2(oldPosition_opengl.y() - _imp->wipeCenter.y() ,
                                          oldPosition_opengl.x() - _imp->wipeCenter.x());
            _imp->mixAmount -= (angle - prevAngle);
            _imp->mixAmount = std::max(0.,std::min(_imp->mixAmount,1.));
            mustRedraw = true;
            
        }   break;
        case ROTATING_WIPE_HANDLE: {
            QMutexLocker l(&_imp->wipeControlsMutex);
            double angle = std::atan2(zoomPos.y() - _imp->wipeCenter.y() , zoomPos.x() - _imp->wipeCenter.x());
            _imp->wipeAngle = angle;
            double mpi2 = M_PI / 2.;
            double closestPI2 = mpi2 * std::floor((_imp->wipeAngle + M_PI / 4.) / mpi2);
            if (std::fabs(_imp->wipeAngle - closestPI2) < 0.1) {
                // snap to closest multiple of PI / 2.
                _imp->wipeAngle = closestPI2;
            }

            mustRedraw = true;
        }   break;
        case PICKING_COLOR: {
            pickColor(newClick.x(), newClick.y());
            mustRedraw = true;
        }   break;
        case BUILDING_PICKER_RECTANGLE: {
            QPointF btmRight = _imp->pickerRect.bottomRight();
            btmRight.rx() -= dxSinceLastMove;
            btmRight.ry() -= dySinceLastMove;
            _imp->pickerRect.setBottomRight(btmRight);
            mustRedraw = true;
        } break;
        default: {
            if (_imp->overlay &&
                _imp->viewerTab->notifyOverlaysPenMotion(1 << mipMapLevel, 1 << mipMapLevel,QMouseEventLocalPos(event), zoomPos)) {
                mustRedraw = true;
            }
        } break;
    }
  
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
}


void ViewerGL::mouseDoubleClickEvent(QMouseEvent* event)
{
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    QPointF pos_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        pos_opengl = _imp->zoomCtx.toZoomCoordinates(event->x(),event->y());
    }
    if (_imp->viewerTab->notifyOverlaysPenDoubleClick(1 << mipMapLevel, 1 << mipMapLevel,QMouseEventLocalPos(event), pos_opengl)) {
        updateGL();
    }
    QGLWidget::mouseDoubleClickEvent(event);
}

void ViewerGL::updateColorPicker(int textureIndex,int x,int y)
{
    if (_imp->pickerState != PICKER_INACTIVE) {
        return;
    }

    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (!displayingImage() && _imp->infoViewer[textureIndex]->colorAndMouseVisible()) {
        _imp->infoViewer[textureIndex]->hideColorAndMouseInfo();
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
    QPointF imgPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPos = _imp->zoomCtx.toZoomCoordinates(pos.x(), pos.y());
    }
    
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool picked = false;
    
    RectI rod = getRoD(0);
    Format dispW = getDisplayWindow();
    if (imgPos.x() >= rod.left() &&
        imgPos.x() < rod.right() &&
        imgPos.y() >= rod.bottom() &&
        imgPos.y() < rod.top() &&
        pos.x() >= 0 && pos.x() < width() &&
        pos.y() >= 0 && pos.y() < height())
    {
        ///if the clip to project format is enabled, make sure it is in the project format too
        bool clipping = isClippingImageToProjectWindow();
        if ((clipping &&
            imgPos.x() >= dispW.left() &&
             imgPos.x() < dispW.right() &&
             imgPos.y() >= dispW.bottom() &&
             imgPos.y() < dispW.top()) || !clipping) {
            unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
            if (mipMapLevel != 0) {
                imgPos /= (1 << mipMapLevel);
            }
            picked = _imp->viewerTab->getInternalNode()->getColorAt(imgPos.x(), imgPos.y(), &r, &g, &b, &a, linear,textureIndex);
        }
        
    }
    if (!picked) {
		if (_imp->infoViewer[textureIndex]->colorAndMouseVisible()) {
			_imp->infoViewer[textureIndex]->hideColorAndMouseInfo();
		}
    } else {
		if (!_imp->infoViewer[textureIndex]->colorAndMouseVisible()) {
			_imp->infoViewer[textureIndex]->showColorAndMouseInfo();
		}
        _imp->infoViewer[textureIndex]->setColor(r,g,b,a);
        emit infoColorUnderMouseChanged();
    }
}

void ViewerGL::wheelEvent(QWheelEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (event->orientation() != Qt::Vertical) {
        return;
    }
    
    _imp->viewerTab->getGui()->selectNode(_imp->viewerTab->getGui()->getApp()->getNodeGui(_imp->viewerTab->getInternalNode()->getNode()));

    const double zoomFactor_min = 0.01;
    const double zoomFactor_max = 1024.;
    double zoomFactor;
    double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates(event->x(), event->y());
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
    int zoomValue = (int)(100*zoomFactor);
    if (zoomValue == 0) {
        zoomValue = 1; // sometimes, floor(100*0.01) makes 0
    }
    assert(zoomValue > 0);
    emit zoomChanged(zoomValue);


    if (displayingImage()) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
    }
    updateGL();
    
    
}

void ViewerGL::zoomSlot(int v)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    
        assert(v > 0);
        double newZoomFactor = v/100.;
        if(newZoomFactor < 0.01) {
            newZoomFactor = 0.01;
        } else if (newZoomFactor > 1024.) {
            newZoomFactor = 1024.;
        }
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        double scale = newZoomFactor / _imp->zoomCtx.factor();
        double centerX = (_imp->zoomCtx.left() + _imp->zoomCtx.right())/2.;
        double centerY = (_imp->zoomCtx.top() + _imp->zoomCtx.bottom())/2.;
        _imp->zoomCtx.zoom(centerX, centerY, scale);
    }

    if(displayingImage()){
        // appPTR->clearPlaybackCache();
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
    } else {
        updateGL();
    }
}

void ViewerGL::zoomSlot(QString str)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    str.remove(QChar('%'));
    int v = str.toInt();
    assert(v > 0);
    zoomSlot(v);
}



void ViewerGL::fitImageToFormat()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    double w,h,zoomPAR;
    
    const Format& format = _imp->currentViewerInfos[0].getDisplayWindow();
    h = format.height();
    w = format.width();
    zoomPAR = format.getPixelAspect();
    
    assert(h > 0. && w > 0.);
    
    double old_zoomFactor;
    double zoomFactor;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        old_zoomFactor = _imp->zoomCtx.factor();
#if 1
        // set the PAR first
        _imp->zoomCtx.setZoom(0., 0., 1., zoomPAR);
        // leave 4% of margin around
        _imp->zoomCtx.fit(-0.02*w, 1.02*w, -0.02*h, 1.02*h);
        zoomFactor = _imp->zoomCtx.factor();
#else
        // leave 5% of margin around
        zoomFactor = 0.95 * std::min(_imp->zoomCtx.screenWidth()/w, _imp->zoomCtx.screenHeight()/h);
        zoomFactor = std::max(0.01, std::min(zoomFactor, 1024.));
        double zoomLeft = w/2.f - (_imp->zoomCtx.screenWidth()/(2.*zoomFactor));
        double zoomBottom = h/2.f - (_imp->zoomCtx.screenHeight()/(2.*zoomFactor)) * zoomPAR;
        _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomPAR);
#endif
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

}


/**
 *@brief Turns on the overlays on the viewer.
 **/
void ViewerGL::turnOnOverlay()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->overlay=true;
}

/**
 *@brief Turns off the overlays on the viewer.
 **/
void ViewerGL::turnOffOverlay()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->overlay=false;
}

void ViewerGL::setInfoViewer(InfoViewerWidget* i,int textureIndex )
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->infoViewer[textureIndex] = i;

}


void ViewerGL::disconnectViewer()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (displayingImage()) {
        setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),0);
        setRegionOfDefinition(_imp->blankViewerInfos.getRoD(),1);
    }
    resetWipeControls();
    clearViewer();
}



/*The dataWindow of the currentFrame(BBOX)*/
RectI ViewerGL::getRoD(int textureIndex) const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->currentViewerInfos[textureIndex].getRoD();
}

/*The displayWindow of the currentFrame(Resolution)
 This is the same for both as we only use the display window as to indicate the project window.*/
Format ViewerGL::getDisplayWindow() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    return _imp->currentViewerInfos[0].getDisplayWindow();
}


void ViewerGL::setRegionOfDefinition(const RectI& rod,int textureIndex)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->currentViewerInfos[textureIndex].setRoD(rod);
    if (textureIndex == 0) {
        emit infoDataWindow1Changed();
    } else {
        assert(textureIndex == 1);
        emit infoDataWindow2Changed();
    }
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].append(QString::number(rod.left()));
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].append(",");
    _imp->currentViewerInfos_btmLeftBBOXoverlay[textureIndex].append(QString::number(rod.bottom()));
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].append(QString::number(rod.right()));
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].append(",");
    _imp->currentViewerInfos_topRightBBOXoverlay[textureIndex].append(QString::number(rod.top()));
}


void ViewerGL::onProjectFormatChanged(const Format& format)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->blankViewerInfos.setDisplayWindow(format);
    _imp->blankViewerInfos.setRoD(format);
    emit infoResolutionChanged();
    
    _imp->currentViewerInfos[0].setDisplayWindow(format);
    _imp->currentViewerInfos[1].setDisplayWindow(format);
    _imp->currentViewerInfos_resolutionOverlay.clear();
    _imp->currentViewerInfos_resolutionOverlay.append(QString::number(format.width()));
    _imp->currentViewerInfos_resolutionOverlay.append("x");
    _imp->currentViewerInfos_resolutionOverlay.append(QString::number(format.height()));
    
    
    if (!_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject()) {
        fitImageToFormat();
    }
    
    if(displayingImage()) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
    }

    if (!_imp->isUserRoISet) {
        {
            QMutexLocker l(&_imp->userRoIMutex);
            _imp->userRoI = format;
        }
        _imp->isUserRoISet = true;
    }
}

void ViewerGL::setClipToDisplayWindow(bool b)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    {
        QMutexLocker l(&_imp->clipToDisplayWindowMutex);
        _imp->clipToDisplayWindow = b;
    }
    ViewerInstance* viewer = _imp->viewerTab->getInternalNode();
    assert(viewer);
    if (viewer->getUiContext() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject()) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
    }
}

bool ViewerGL::isClippingImageToProjectWindow() const
{
    // MT-SAFE
    QMutexLocker l(&_imp->clipToDisplayWindowMutex);
    return _imp->clipToDisplayWindow;
}

/*display black in the viewer*/
void ViewerGL::clearViewer()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->activeTextures[0] = 0;
    _imp->activeTextures[1] = 0;
    updateGL();
}

/*overload of QT enter/leave/resize events*/
void ViewerGL::focusInEvent(QFocusEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if(_imp->viewerTab->notifyOverlaysFocusGained(scale,scale)){
        updateGL();
    }
    QGLWidget::focusInEvent(event);
}

void ViewerGL::focusOutEvent(QFocusEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
	if (!_imp->viewerTab->getGui()) {
		return;
	}

    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if(_imp->viewerTab->notifyOverlaysFocusLost(scale,scale)){
        updateGL();
    }
    QGLWidget::focusOutEvent(event);
}

void ViewerGL::enterEvent(QEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    setFocus();
    QGLWidget::enterEvent(event);
}

void ViewerGL::leaveEvent(QEvent *event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->infoViewer[0]->hideColorAndMouseInfo();
    _imp->infoViewer[1]->hideColorAndMouseInfo();
    QGLWidget::leaveEvent(event);
}

void ViewerGL::resizeEvent(QResizeEvent* event)
{ // public to hack the protected field
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    QGLWidget::resizeEvent(event);
}

void ViewerGL::keyPressEvent(QKeyEvent* event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (event->key() == Qt::Key_O) {
        toggleOverlays();
    }
    
    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    bool accept = false;
    if(event->isAutoRepeat()){
        if(_imp->viewerTab->notifyOverlaysKeyRepeat(scale,scale,event)){
            accept = true;
            updateGL();
        }
    }else{
        if(_imp->viewerTab->notifyOverlaysKeyDown(scale,scale,event)){
            accept = true;
            updateGL();
        }
    }
    if (accept) {
        event->accept();
    } else {
        event->ignore();
    }
}


void ViewerGL::keyReleaseEvent(QKeyEvent* event)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    unsigned int scale = 1 << getInternalNode()->getMipMapLevel();
    if(_imp->viewerTab->notifyOverlaysKeyUp(scale,scale,event)){
        updateGL();
    }
}


OpenGLViewerI::BitDepth ViewerGL::getBitDepth() const
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



void ViewerGL::populateMenu()
{
    // always running in the main thread
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
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(QGLContext::currentContext() == context());

    if (string.isEmpty()) {
        return;
    }

    glMatrixMode (GL_PROJECTION);
    glPushMatrix(); // save GL_PROJECTION
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);

    glMatrixMode(GL_MODELVIEW);
    QPointF pos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        pos = _imp->zoomCtx.toWidgetCoordinates(x, y);
    }
    glCheckError();
    _imp->textRenderer.renderText(pos.x(),h-pos.y(),string,color,font);
    glCheckError();
    
    glMatrixMode (GL_PROJECTION);
    glPopMatrix(); // restore GL_PROJECTION
    glMatrixMode(GL_MODELVIEW);
}

void ViewerGL::setPersistentMessage(int type,const QString& message)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->persistentMessageType = type;
    _imp->persistentMessage = message;
    _imp->displayPersistentMessage = true;
    updateGL();
}

void ViewerGL::clearPersistentMessage()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    if (!_imp->displayPersistentMessage) {
        return;
    }
    _imp->displayPersistentMessage = false;
    updateGL();
}

void ViewerGL::getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomPAR) const
{
    // MT-SAFE
    QMutexLocker l(&_imp->zoomCtxMutex);
    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomPAR = _imp->zoomCtx.par();
}

void ViewerGL::setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomPAR)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    QMutexLocker l(&_imp->zoomCtxMutex);
    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomPAR);
}

void ViewerGL::setUserRoIEnabled(bool b)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    {
        QMutexLocker(&_imp->userRoIMutex);
        _imp->userRoIEnabled = b;
    }
    if (displayingImage()) {
        _imp->viewerTab->getInternalNode()->refreshAndContinueRender(false);
    }
    update();
}

bool ViewerGL::isNearByUserRoITopEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    int length = std::min(roi.x2 - roi.x1 - 10,(int)(USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth) * 2);
    RectI r(roi.x1 + length / 2,
              roi.y2 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight,
              roi.x2 - length / 2,
              roi.y2 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight);
    
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoIRightEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    int length = std::min(roi.y2 - roi.y1 - 10,(int)(USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight) * 2);
    RectI r(roi.x2 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
                  roi.y1 + length / 2,
                  roi.x2 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
                  roi.y2 - length / 2);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoILeftEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    int length = std::min(roi.y2 - roi.y1 - 10,(int)(USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight) * 2);
    RectI r(roi.x1 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
                  roi.y1 + length / 2,
                  roi.x1 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
                  roi.y2 - length / 2);
    
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoIBottomEdge(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    int length = std::min(roi.x2 - roi.x1 - 10,(int)(USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth) * 2);
    RectI r(roi.x1 + length / 2,
                  roi.y1 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight,
                  roi.x2 - length / 2,
                  roi.y1 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoIMiddleHandle(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    RectI r((roi.x1 + roi.x2) / 2 - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  (roi.y1 + roi.y2) / 2 - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
                  (roi.x1 + roi.x2) / 2 + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  (roi.y1 + roi.y2) / 2 + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoITopLeft(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    RectI r(roi.x1 - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y2 - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
                  roi.x1  + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y2  + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoITopRight(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    RectI r(roi.x2 - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y2 - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
                  roi.x2  + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y2  + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoIBottomRight(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    RectI r(roi.x2 - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y1 - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
                  roi.x2  + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y1  + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isNearByUserRoIBottomLeft(const RectI& roi,const QPointF& zoomPos, double zoomScreenPixelWidth, double zoomScreenPixelHeight)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    RectI r(roi.x1 - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y1 - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
                  roi.x1  + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
                  roi.y1  + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);
    return r.contains(zoomPos.x(),zoomPos.y());
}

bool ViewerGL::isUserRegionOfInterestEnabled() const
{
    // MT-SAFE
    QMutexLocker(&_imp->userRoIMutex);
    return _imp->userRoIEnabled;
}

RectI ViewerGL::getUserRegionOfInterest() const
{
    // MT-SAFE
    QMutexLocker(&_imp->userRoIMutex);
    return _imp->userRoI;
}

void ViewerGL::setUserRoI(const RectI& r)
{
    // MT-SAFE
    QMutexLocker(&_imp->userRoIMutex);
    _imp->userRoI = r;
}

/**
* @brief Swap the OpenGL buffers.
**/
void ViewerGL::swapOpenGLBuffers()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    swapBuffers();
}

/**
 * @brief Repaint
**/
void ViewerGL::redraw()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    update();
}

/**
* @brief Returns the width and height of the viewport in window coordinates.
**/
void ViewerGL::getViewportSize(double &width, double &height) const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    QMutexLocker l(&_imp->zoomCtxMutex);
    width = _imp->zoomCtx.screenWidth();
    height = _imp->zoomCtx.screenHeight();
}

/**
* @brief Returns the pixel scale of the viewport.
**/
void ViewerGL::getPixelScale(double& xScale, double& yScale) const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    QMutexLocker l(&_imp->zoomCtxMutex);
    xScale = _imp->zoomCtx.screenPixelWidth();
    yScale = _imp->zoomCtx.screenPixelHeight();
}

/**
* @brief Returns the colour of the background (i.e: clear color) of the viewport.
**/
void ViewerGL::getBackgroundColour(double &r, double &g, double &b) const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    r = _imp->clearColor.redF();
    g = _imp->clearColor.greenF();
    b = _imp->clearColor.blueF();
}

void ViewerGL::makeOpenGLcontextCurrent()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    makeCurrent();
}

void ViewerGL::onViewerNodeNameChanged(const QString& name)
{
    // always running in the main thread
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
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
	if (_imp->viewerTab->getGui()) {
		_imp->viewerTab->getGui()->removeViewerTab(_imp->viewerTab, true,true);
	}
}

int ViewerGL::getCurrentView() const
{
    // MT-SAFE
    ///protected in viewerTab (which is const)
    return _imp->viewerTab->getCurrentView();
}

ViewerInstance* ViewerGL::getInternalNode() const {
    return _imp->viewerTab->getInternalNode();
}

ViewerTab* ViewerGL::getViewerTab() const {
    return _imp->viewerTab;
}

namespace {
static float clamp(float v)
{
    return std::min(std::max(v,0.f),1.f);
}
}

bool ViewerGL::pickColor(double x,double y)
{
    
    float r,g,b,a;
    QPointF imgPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPos = _imp->zoomCtx.toZoomCoordinates(x, y);
    }
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    if (mipMapLevel != 0) {
        imgPos /= (1 << mipMapLevel);
        
    }
    
    _imp->lastPickerPos = imgPos;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool ret = false;
    for (int i = 0; i< 2 ;++i ) {
        bool picked = _imp->viewerTab->getInternalNode()->getColorAt(imgPos.x(), imgPos.y(), &r, &g, &b, &a, linear,i);
        if (picked) {
            QColor pickerColor;
            pickerColor.setRedF(clamp(r));
            pickerColor.setGreenF(clamp(g));
            pickerColor.setBlueF(clamp(b));
            pickerColor.setAlphaF(clamp(a));
            _imp->viewerTab->getGui()->setColorPickersColor(pickerColor);
            _imp->infoViewer[i]->setColor(r,g,b,a);
            emit infoColorUnderMouseChanged();
            ret = true;
        } else {
            if (_imp->infoViewer[i]->colorAndMouseVisible()) {
                _imp->infoViewer[i]->hideColorAndMouseInfo();
            }
        }
    }
    return ret;
}

void ViewerGL::updateInfoWidgetColorPicker(const QPointF& imgPos,const QPoint& widgetPos,
                                           int width,int height,const RectI& rod,const RectI& dispW,
                                           int texIndex)
{
    
    
    if (_imp->activeTextures[texIndex] &&
        imgPos.x() >= rod.left() &&
        imgPos.x() < rod.right() &&
        imgPos.y() >= rod.bottom() &&
        imgPos.y() < rod.top() &&
        widgetPos.x() >= 0 && widgetPos.x() < width &&
        widgetPos.y() >= 0 && widgetPos.y() < height) {
        
        ///if the clip to project format is enabled, make sure it is in the project format too
        if (isClippingImageToProjectWindow() &&
            (imgPos.x() < dispW.left() ||
            imgPos.x() >= dispW.right() ||
            imgPos.y() < dispW.bottom() ||
            imgPos.y() >= dispW.top())) {
                if (_imp->infoViewer[texIndex]->colorAndMouseVisible()) {
                    _imp->infoViewer[texIndex]->hideColorAndMouseInfo();
                }
            } else {
                
                if (_imp->pickerState == PICKER_INACTIVE) {
                    boost::shared_ptr<VideoEngine> videoEngine = _imp->viewerTab->getInternalNode()->getVideoEngine();
                    if (!videoEngine->isWorking()) {
                        updateColorPicker(texIndex,widgetPos.x(),widgetPos.y());
                    }
                } else if (_imp->pickerState == PICKER_POINT || _imp->pickerState == PICKER_RECTANGLE) {
                    if (!_imp->infoViewer[texIndex]->colorAndMouseVisible()) {
                        _imp->infoViewer[texIndex]->showColorAndMouseInfo();
                    }
                } else {
                    ///unkwn state
                    assert(false);
                }
                
                _imp->infoViewer[texIndex]->setMousePos(QPoint((int)(imgPos.x()),(int)(imgPos.y())));
                emit infoMousePosChanged();
            }
    } else {
        if (_imp->infoViewer[texIndex]->colorAndMouseVisible()) {
            _imp->infoViewer[texIndex]->hideColorAndMouseInfo();
        }
    }


  
}

void ViewerGL::updateRectangleColorPicker()
{
    
    float rSum = 0.,gSum = 0,bSum = 0,aSum = 0;
    float r,g,b,a;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    
    int samples = 0;
    
    QPointF topLeft = _imp->pickerRect.topLeft();
    QPointF btmRight = _imp->pickerRect.bottomRight();
    int left = std::min(topLeft.x(),btmRight.x());
    int btm = std::min(topLeft.y(),btmRight.y());
    int right = std::max(topLeft.x(),btmRight.x());
    int top = std::max(topLeft.y(),btmRight.y());
    for (int y = btm; y <= top; ++y) {
        for (int x = left; x <= right; ++x) {
            int rx = x,ry = y;
            if (mipMapLevel != 0) {
                rx /= (1 << mipMapLevel);
                ry /= (1 << mipMapLevel);
            }
            for (int i = 0; i < 2; ++i) {
                bool picked = _imp->viewerTab->getInternalNode()->getColorAt(rx,ry, &r, &g, &b, &a, linear,i);
                if (picked) {
                    rSum += r;
                    gSum += g;
                    bSum += b;
                    aSum += a;
                    ++samples;
                }
            }
        }
    }
    if (samples != 0) {
        rSum /= (float)samples;
        gSum /= (float)samples;
        bSum /= (float)samples;
        aSum /= (float)samples;
    }
    
    for (int i = 0; i< 2; ++i) {
        if (!_imp->infoViewer[i]->colorAndMouseVisible()) {
            _imp->infoViewer[i]->showColorAndMouseInfo();
            
        }
        _imp->infoViewer[i]->setColor(rSum,gSum,bSum,aSum);
    }
    emit infoColorUnderMouseChanged();
    
    QColor pickerColor;
    pickerColor.setRedF(clamp(rSum));
    pickerColor.setGreenF(clamp(gSum));
    pickerColor.setBlueF(clamp(bSum));
    pickerColor.setAlphaF(clamp(aSum));
    _imp->viewerTab->getGui()->setColorPickersColor(pickerColor);

}

void ViewerGL::resetWipeControls()
{
    RectI rod;
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

bool ViewerGL::Implementation::isNearbyWipeCenter(const QPointF& pos,double tolerance) const
{
    QMutexLocker l(&wipeControlsMutex);
    if (pos.x() >= (wipeCenter.x() - tolerance) && pos.x() <= (wipeCenter.x() + tolerance) &&
        pos.y() >= (wipeCenter.y() - tolerance) && pos.y() <= (wipeCenter.y() + tolerance)) {
        return true;
    }
    return false;
}

bool ViewerGL::Implementation::isNearbyWipeRotateBar(const QPointF& pos,double tolerance) const
{
    
    double rotateLenght,rotateOffset;
    {
        QMutexLocker l(&zoomCtxMutex);
        rotateLenght = WIPE_ROTATE_HANDLE_LENGTH / zoomCtx.factor();
        rotateOffset = WIPE_ROTATE_OFFSET / zoomCtx.factor();
    }
    QMutexLocker l(&wipeControlsMutex);
    QPointF outterPoint;
    outterPoint.setX(wipeCenter.x() + std::cos(wipeAngle) * (rotateLenght - rotateOffset));
    outterPoint.setY(wipeCenter.y() + std::sin(wipeAngle) * (rotateLenght - rotateOffset));
    if (((pos.y() >= (wipeCenter.y() - tolerance) && pos.y() <= (outterPoint.y() + tolerance)) ||
         (pos.y() >= (outterPoint.y() - tolerance) && pos.y() <= (wipeCenter.y() + tolerance))) &&
        ((pos.x() >= (wipeCenter.x() - tolerance) && pos.x() <= (outterPoint.x() + tolerance)) ||
         (pos.x() >= (outterPoint.x() - tolerance) && pos.x() <= (wipeCenter.x() + tolerance)))) {
            Point a;
            a.x = (outterPoint.x() - wipeCenter.x());
            a.y = (outterPoint.y() - wipeCenter.y());
            double norm = sqrt(a.x * a.x + a.y * a.y);
            
            ///The point is in the bounding box of the segment, if it is vertical it must be on the segment anyway
            if (norm == 0) {
                return false;
            }
            
            a.x /= norm;
            a.y /= norm;
            Point b;
            b.x = (pos.x() - wipeCenter.x());
            b.y = (pos.y() - wipeCenter.y());
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

bool ViewerGL::Implementation::isNearbyWipeMixHandle(const QPointF& pos,double tolerance) const
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
    if (pos.x() >= (mixPos.x() - tolerance) && pos.x() <= (mixPos.x() + tolerance) &&
        pos.y() >= (mixPos.y() - tolerance) && pos.y() <= (mixPos.y() + tolerance)) {
        return true;
    }
    return false;

}

bool ViewerGL::isWipeHandleVisible() const
{
    return _imp->viewerTab->getCompositingOperator() != OPERATOR_NONE;
}

void ViewerGL::setZoomOrPannedSinceLastFit(bool enabled)
{
    QMutexLocker l(&_imp->zoomCtxMutex);
    _imp->zoomOrPannedSinceLastFit = enabled;
}

bool ViewerGL::getZoomOrPannedSinceLastFit() const
{
    QMutexLocker l(&_imp->zoomCtxMutex);
    return _imp->zoomOrPannedSinceLastFit;
}

Natron::ViewerCompositingOperator ViewerGL::getCompositingOperator() const
{
    return _imp->viewerTab->getCompositingOperator();
}
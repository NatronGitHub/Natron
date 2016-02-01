/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerGLPrivate.h"

#include <cassert>
#include <algorithm> // min, max
#include <cmath> // sin, cos
#include <stdexcept>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QApplication> // qApp
#include <QtOpenGL/QGLShaderProgram>

#include "Engine/Lut.h" // Color
#include "Engine/Settings.h"

#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h" // appFont
#include "Gui/Menu.h"
#include "Gui/Texture.h"
#include "Gui/ViewerTab.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif
#ifndef M_PI_4
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4           */
#endif



#define MAX_MIP_MAP_LEVELS 20

NATRON_NAMESPACE_ENTER;

/*This class is the the core of the viewer : what displays images, overlays, etc...
   Everything related to OpenGL will (almost always) be in this class */
ViewerGL::Implementation::Implementation(ViewerGL* this_, ViewerTab* parent)
: _this(this_)
, pboIds()
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
, blankViewerInfo()
, displayingImageGain()
, displayingImageGamma()
, displayingImageOffset()
, displayingImageMipMapLevel()
, displayingImagePremult()
, displayingImageLut(eViewerColorSpaceSRGB)
, ms(eMouseStateUndefined)
, hs(eHoverStateNothing)
, textRenderingColor(200,200,200,255)
, displayWindowOverlayColor(125,125,125,255)
, rodOverlayColor(100,100,100,255)
, textFont( new QFont(appFont,appFontSize) )
, overlay(true)
, supportsGLSL(true)
, updatingTexture(false)
, clearColor(0,0,0,255)
, menu( new Menu(_this) )
, persistentMessages()
, persistentMessageType(0)
, displayPersistentMessage(false)
, textRenderer()
, lastMousePosition()
, lastDragStartPos()
, hasMovedSincePress(false)
, currentViewerInfo()
, projectFormatMutex()
, projectFormat()
, currentViewerInfo_btmLeftBBOXoverlay()
, currentViewerInfo_topRightBBOXoverlay()
, currentViewerInfo_resolutionOverlay()
, pickerState(ePickerStateInactive)
, lastPickerPos()
, userRoIEnabled(false) // protected by mutex
, userRoI() // protected by mutex
, buildUserRoIOnNextPress(false)
, draggedUserRoI()
, zoomCtx() // protected by mutex
, clipToDisplayWindow(false) // protected by mutex
, wipeControlsMutex()
, mixAmount(1.) // protected by mutex
, wipeAngle(M_PI_2) // protected by mutex
, wipeCenter()
, selectionRectangle()
, checkerboardTextureID(0)
, checkerboardTileSize(0)
, savedTexture(0)
, prevBoundTexture(0)
, lastRenderedImageMutex()
, lastRenderedTiles()
, memoryHeldByLastRenderedImages()
, sizeH()
, pointerTypeOnPress(ePenTypePen)
, subsequentMousePressIsTablet(false)
, pressureOnPress(1.)
, pressureOnRelease(1.)
, wheelDeltaSeekFrame(0)
, lastTextureRoi()
, isUpdatingTexture(false)
, renderOnPenUp(false)
{
    infoViewer[0] = 0;
    infoViewer[1] = 0;
    displayTextures[0] = 0;
    displayTextures[1] = 0;
    activeTextures[0] = 0;
    activeTextures[1] = 0;
    memoryHeldByLastRenderedImages[0] = memoryHeldByLastRenderedImages[1] = 0;
    displayingImageGain[0] = displayingImageGain[1] = 1.;
    displayingImageGamma[0] = displayingImageGamma[1] = 1.;
    displayingImageOffset[0] = displayingImageOffset[1] = 0.;
    for (int i = 0; i < 2 ; ++i) {
        displayingImageTime[i] = 0;
        displayingImageMipMapLevel[i] = 0;
        lastRenderedTiles[i].resize(MAX_MIP_MAP_LEVELS);
    }
    assert( qApp && qApp->thread() == QThread::currentThread() );
    //menu->setFont( QFont(appFont,appFontSize) );

    //        QDesktopWidget* desktop = QApplication::desktop();
    //        QRect r = desktop->screenGeometry();
    //        sizeH = r.size();
    sizeH.setWidth(10000);
    sizeH.setHeight(10000);
}


ViewerGL::Implementation::~Implementation()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _this->makeCurrent();

    if (this->shaderRGB) {
        this->shaderRGB->removeAllShaders();
        delete this->shaderRGB;
    }
    if (this->shaderBlack) {
        this->shaderBlack->removeAllShaders();
        delete this->shaderBlack;
    }
    delete this->displayTextures[0];
    delete this->displayTextures[1];
    glCheckError();
    for (U32 i = 0; i < this->pboIds.size(); ++i) {
        glDeleteBuffers(1,&this->pboIds[i]);
    }
    glCheckError();
    glDeleteBuffers(1, &this->vboVerticesId);
    glDeleteBuffers(1, &this->vboTexturesId);
    glDeleteBuffers(1, &this->iboTriangleStripId);
    glCheckError();
    delete this->textFont;
    glDeleteTextures(1, &this->checkerboardTextureID);
}

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


void
ViewerGL::Implementation::drawRenderingVAO(unsigned int mipMapLevel,
                                           int textureIndex,
                                           ViewerGL::DrawPolygonModeEnum polygonMode)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _this->context() );

    bool useShader = _this->getBitDepth() != eImageBitDepthByte && this->supportsGLSL;


    ///the texture rectangle in image coordinates. The values in it are multiples of tile size.
    ///
    const TextureRect &r = this->activeTextures[textureIndex]->getTextureRect();

    ///This is the coordinates in the image being rendered where datas are valid, this is in pixel coordinates
    ///at the time we initialize it but we will convert it later to canonical coordinates. See 1)
    RectI texRect(r.x1,r.y1,r.x2,r.y2);

    const double par = r.par;

    RectD canonicalTexRect;
    texRect.toCanonical_noClipping(mipMapLevel,par/*, rod*/, &canonicalTexRect);

    ///the RoD of the image in canonical coords.
    RectD rod = _this->getRoD(textureIndex);

    bool clipToDisplayWindow;
    {
        QMutexLocker l(&this->clipToDisplayWindowMutex);
        clipToDisplayWindow = this->clipToDisplayWindow;
    }

    RectD rectClippedToRoI(canonicalTexRect);

    if (clipToDisplayWindow) {
        RectD canonicalProjectFormat;
        this->getProjectFormatCanonical(canonicalProjectFormat);
        rod.intersect(canonicalProjectFormat, &rod);
        rectClippedToRoI.intersect(canonicalProjectFormat, &rectClippedToRoI);
    }


    //if user RoI is enabled, clip the rod to that roi
    bool userRoiEnabled;
    {
        QMutexLocker l(&this->userRoIMutex);
        userRoiEnabled = this->userRoIEnabled;
    }


    ////The texture real size (r.w,r.h) might be slightly bigger than the actual
    ////pixel coordinates bounds r.x1,r.x2 r.y1 r.y2 because we clipped these bounds against the bounds
    ////in the ViewerInstance::renderViewer function. That means we need to draw actually only the part of
    ////the texture that contains the bounds.
    ////Notice that r.w and r.h are scaled to the closest Po2 of the current scaling factor, so we need to scale it up
    ////So it is in the same coordinates as the bounds.
    ///Edit: we no longer divide by the closestPo2 since the viewer now computes images at lower resolution by itself, the drawing
    ///doesn't need to be scaled.

    if (userRoiEnabled) {
        {
            QMutexLocker l(&this->userRoIMutex);
            //if the userRoI isn't intersecting the rod, just don't render anything
            if ( !rod.intersect(this->userRoI,&rod) ) {
                return;
            }
        }
        rectClippedToRoI.intersect(rod, &rectClippedToRoI);
        //clipTexCoords<RectD>(canonicalTexRect,rectClippedToRoI,texBottom,texTop,texLeft,texRight);
    }

    if (polygonMode != eDrawPolygonModeWhole) {
        /// draw only  the plane defined by the wipe handle
        QPolygonF polygonPoints,polygonTexCoords;
        RectD floatRectClippedToRoI;
        floatRectClippedToRoI.x1 = rectClippedToRoI.x1;
        floatRectClippedToRoI.y1 = rectClippedToRoI.y1;
        floatRectClippedToRoI.x2 = rectClippedToRoI.x2;
        floatRectClippedToRoI.y2 = rectClippedToRoI.y2;
        Implementation::WipePolygonEnum polyType = this->getWipePolygon(floatRectClippedToRoI, polygonPoints, polygonMode == eDrawPolygonModeWipeRight);

        if (polyType == Implementation::eWipePolygonEmpty) {
            ///don't draw anything
            return;
        } else if (polyType == Implementation::eWipePolygonPartial) {
            this->getPolygonTextureCoordinates(polygonPoints, canonicalTexRect, polygonTexCoords);

            this->bindTextureAndActivateShader(textureIndex, useShader);

            glBegin(GL_POLYGON);
            for (int i = 0; i < polygonTexCoords.size(); ++i) {
                const QPointF & tCoord = polygonTexCoords[i];
                const QPointF & vCoord = polygonPoints[i];
                glTexCoord2d( tCoord.x(), tCoord.y() );
                glVertex2d( vCoord.x(), vCoord.y() );
            }
            glEnd();

            this->unbindTextureAndReleaseShader(useShader);

        } else {
            ///draw the all polygon as usual
            polygonMode = eDrawPolygonModeWhole;
        }
    }

    if (polygonMode == eDrawPolygonModeWhole) {



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

        //        GLfloat texBottom =  0;
        //        GLfloat texTop =  (GLfloat)(r.y2 - r.y1)  / (GLfloat)(r.h /** r.closestPo2*/);
        //        GLfloat texLeft = 0;
        //        GLfloat texRight = (GLfloat)(r.x2 - r.x1)  / (GLfloat)(r.w /** r.closestPo2*/);
        GLfloat texBottom = (GLfloat)(rectClippedToRoI.y1 - canonicalTexRect.y1)  / canonicalTexRect.height();
        GLfloat texTop = (GLfloat)(rectClippedToRoI.y2 - canonicalTexRect.y1)  / canonicalTexRect.height();
        GLfloat texLeft = (GLfloat)(rectClippedToRoI.x1 - canonicalTexRect.x1)  / canonicalTexRect.width();
        GLfloat texRight = (GLfloat)(rectClippedToRoI.x2 - canonicalTexRect.x1)  / canonicalTexRect.width();


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


        if (this->viewerTab->isCheckerboardEnabled()) {
            this->drawCheckerboardTexture(rod);
        }

        this->bindTextureAndActivateShader(textureIndex, useShader);

        glCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, this->vboVerticesId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 32 * sizeof(GLfloat), vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, this->vboTexturesId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 32 * sizeof(GLfloat), renderingTextureCoordinates);
        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, 0);

        glDisableClientState(GL_COLOR_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER,0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->iboTriangleStripId);
        glDrawElements(GL_TRIANGLE_STRIP, 28, GL_UNSIGNED_BYTE, 0);
        glCheckErrorIgnoreOSXBug();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glCheckError();
        
        this->unbindTextureAndReleaseShader(useShader);
    }
} // drawRenderingVAO

void
ViewerGL::Implementation::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _this->makeCurrent();
    initAndCheckGlExtensions();
    this->displayTextures[0] = new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE);
    this->displayTextures[1] = new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE);


    // glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &this->vboVerticesId);
    glGenBuffers(1, &this->vboTexturesId);
    glGenBuffers(1, &this->iboTriangleStripId);

    glBindBuffer(GL_ARRAY_BUFFER, this->vboTexturesId);
    glBufferData(GL_ARRAY_BUFFER, 32 * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, this->vboVerticesId);
    glBufferData(GL_ARRAY_BUFFER, 32 * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->iboTriangleStripId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 28 * sizeof(GLubyte), triangleStrip, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError();

    initializeCheckerboardTexture(true);

    if (this->supportsGLSL) {
        _this->initShaderGLSL();
        glCheckError();
    }
    
    glCheckError();
}

static
QString
getOpenGLVersionString()
{
    const char* str = (const char*)glGetString(GL_VERSION);
    QString ret;
    if (str) {
        ret.append(str);
    }

    return ret;
}

static
QString
getGlewVersionString()
{
    const char* str = reinterpret_cast<const char *>( glewGetString(GLEW_VERSION) );
    QString ret;
    if (str) {
        ret.append(str);
    }

    return ret;
}

void
ViewerGL::Implementation::initAndCheckGlExtensions()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _this->context() );
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        Dialogs::errorDialog( tr("OpenGL/GLEW error").toStdString(),
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
        Dialogs::errorDialog( tr("Missing OpenGL requirements").toStdString(),
                            tr("The viewer may not be fully functional. "
                               "This software needs at least OpenGL 1.5 with NPOT textures, GLSL, VBO, PBO, vertex arrays. ").toStdString() );
    }

    this->viewerTab->getGui()->setOpenGLVersion( getOpenGLVersionString() );
    this->viewerTab->getGui()->setGlewVersion( getGlewVersionString() );

    if ( !QGLShaderProgram::hasOpenGLShaderPrograms( _this->context() ) ) {
        // no need to pull out a dialog, it was already presented after the GLEW check above

        //Dialogs::errorDialog("Viewer error","The viewer is unabgile to work without a proper version of GLSL.");
        //cout << "Warning : GLSL not present on this hardware, no material acceleration possible." << endl;
        this->supportsGLSL = false;
    }
}



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

ViewerGL::Implementation::WipePolygonEnum
ViewerGL::Implementation::getWipePolygon(const RectD & texRectClipped,
                                         QPolygonF & polygonPoints,
                                         bool rightPlane) const
{
    ///Compute a second point on the plane separator line
    ///we don't really care how far it is from the center point, it just has to be on the line
    QPointF firstPoint,secondPoint;
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

    xmax = std::cos(angle + M_PI_2) * maxSize;
    ymax = std::sin(angle + M_PI_2) * maxSize;

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
    int validIntersectionsIndex[4];
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
        return ViewerGL::Implementation::eWipePolygonEmpty;
    }

    ///determine the orientation of the planes
    double crossProd  = ( secondPoint.x() - center.x() ) * ( texRectClipped.y1 - center.y() )
                        - ( secondPoint.y() - center.y() ) * ( texRectClipped.x1 - center.x() );
    if (numIntersec == 0) {
        ///the bottom left corner is on the left plane
        if ( (crossProd > 0) && ( (center.x() >= texRectClipped.x2) || (center.y() >= texRectClipped.y2) ) ) {
            ///the plane is invisible because the wipe handle is below or on the left of the texRectClipped
            return rightPlane ? ViewerGL::Implementation::eWipePolygonEmpty : ViewerGL::Implementation::eWipePolygonFull;
        }

        ///otherwise we draw the entire texture as usual
        return rightPlane ? ViewerGL::Implementation::eWipePolygonFull : ViewerGL::Implementation::eWipePolygonEmpty;
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

    return ViewerGL::Implementation::eWipePolygonPartial;
} // getWipePolygon


/**
 * @brief Used to setup the blending mode to draw the first texture
 **/
class BlendSetter
{
    
    bool didBlend;
    
public:
    
    BlendSetter(ImagePremultiplicationEnum premult)
    {
        didBlend = premult != eImagePremultiplicationOpaque;
        if (didBlend) {
            glEnable(GL_BLEND);
        }
        switch (premult) {
            case eImagePremultiplicationPremultiplied:
                glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
                break;
            case eImagePremultiplicationUnPremultiplied:
                glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
                break;
            case eImagePremultiplicationOpaque:
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
ViewerGL::Implementation::drawArcOfCircle(const QPointF & center,
                                          double radiusX,
                                          double radiusY,
                                          double startAngle,
                                          double endAngle)
{
    double alpha = startAngle;
    double x,y;

    {
        GLProtectAttrib a(GL_CURRENT_BIT);

        if ( (hs == eHoverStateWipeMix) || (ms == eMouseStateDraggingWipeMixHandle) ) {
            glColor3f(0, 1, 0);
        }
        glBegin(GL_POINTS);
        while (alpha <= endAngle) {
            x = center.x()  + radiusX * std::cos(alpha);
            y = center.y()  + radiusY * std::sin(alpha);
            glVertex2d(x, y);
            alpha += 0.01;
        }
        glEnd();
    } // GLProtectAttrib a(GL_CURRENT_BIT);
}

void
ViewerGL::Implementation::bindTextureAndActivateShader(int i,
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

void
ViewerGL::Implementation::unbindTextureAndReleaseShader(bool useShader)
{
    if (useShader) {
        shaderRGB->release();
    }
    glCheckError();
    glBindTexture(GL_TEXTURE_2D, prevBoundTexture);
}

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
        qDebug() << "Error when binding shader" << qPrintable( shaderRGB->log() );
    }

    shaderRGB->setUniformValue("Tex", 0);
    shaderRGB->setUniformValue("gain", (float)displayingImageGain[texIndex]);
    shaderRGB->setUniformValue("offset", (float)displayingImageOffset[texIndex]);
    shaderRGB->setUniformValue("lut", (GLint)displayingImageLut);
    float gamma = (displayingImageGamma[texIndex] == 0.) ? 0.f : 1.f / (float)displayingImageGamma[texIndex];
    shaderRGB->setUniformValue("gamma", gamma);
}


bool
ViewerGL::Implementation::isNearbyWipeCenter(const QPointF & pos,
                                             double zoomScreenPixelWidth, double zoomScreenPixelHeight) const
{
    double toleranceX = zoomScreenPixelWidth * 8.;
    double toleranceY = zoomScreenPixelHeight * 8.;
    QMutexLocker l(&wipeControlsMutex);

    if ( ( pos.x() >= (wipeCenter.x() - toleranceX) ) && ( pos.x() <= (wipeCenter.x() + toleranceX) ) &&
         ( pos.y() >= (wipeCenter.y() - toleranceY) ) && ( pos.y() <= (wipeCenter.y() + toleranceY) ) ) {
        return true;
    }

    return false;
}

bool
ViewerGL::Implementation::isNearbyWipeRotateBar(const QPointF & pos,
                                                double zoomScreenPixelWidth, double zoomScreenPixelHeight) const
{
    double toleranceX = zoomScreenPixelWidth * 8.;
    double toleranceY = zoomScreenPixelHeight * 8.;

    
    double rotateX,rotateY,rotateOffsetX,rotateOffsetY;
   
    rotateX = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelWidth;
    rotateY = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelHeight;
    rotateOffsetX = WIPE_ROTATE_OFFSET * zoomScreenPixelWidth;
    rotateOffsetY = WIPE_ROTATE_OFFSET * zoomScreenPixelHeight;
    
    QMutexLocker l(&wipeControlsMutex);
    QPointF outterPoint;

    outterPoint.setX( wipeCenter.x() + std::cos(wipeAngle) * (rotateX - rotateOffsetX) );
    outterPoint.setY( wipeCenter.y() + std::sin(wipeAngle) * (rotateY - rotateOffsetY) );
    if ( ( ( ( pos.y() >= (wipeCenter.y() - toleranceY) ) && ( pos.y() <= (outterPoint.y() + toleranceY) ) ) ||
           ( ( pos.y() >= (outterPoint.y() - toleranceY) ) && ( pos.y() <= (wipeCenter.y() + toleranceY) ) ) ) &&
         ( ( ( pos.x() >= (wipeCenter.x() - toleranceX) ) && ( pos.x() <= (outterPoint.x() + toleranceX) ) ) ||
           ( ( pos.x() >= (outterPoint.x() - toleranceX) ) && ( pos.x() <= (wipeCenter.x() + toleranceX) ) ) ) ) {
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
                                                double zoomScreenPixelWidth, double zoomScreenPixelHeight) const
{
    double toleranceX = zoomScreenPixelWidth * 8.;
    double toleranceY = zoomScreenPixelHeight * 8.;
    
    QMutexLocker l(&wipeControlsMutex);
    ///mix 1 is at rotation bar + pi / 8
    ///mix 0 is at rotation bar + 3pi / 8
    double alphaMix1,alphaMix0,alphaCurMix;

    alphaMix1 = wipeAngle + M_PI_4 / 2;
    alphaMix0 = wipeAngle + 3 * M_PI_4 / 2;
    alphaCurMix = mixAmount * (alphaMix1 - alphaMix0) + alphaMix0;
    QPointF mixPos;
    double mixX = WIPE_MIX_HANDLE_LENGTH * zoomScreenPixelWidth;
    double mixY = WIPE_MIX_HANDLE_LENGTH * zoomScreenPixelHeight;

    mixPos.setX(wipeCenter.x() + std::cos(alphaCurMix) * mixX);
    mixPos.setY(wipeCenter.y() + std::sin(alphaCurMix) * mixY);
    if ( ( pos.x() >= (mixPos.x() - toleranceX) ) && ( pos.x() <= (mixPos.x() + toleranceX) ) &&
         ( pos.y() >= (mixPos.y() - toleranceY) ) && ( pos.y() <= (mixPos.y() + toleranceY) ) ) {
        return true;
    }

    return false;
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

NATRON_NAMESPACE_EXIT;

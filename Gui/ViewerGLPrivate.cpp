/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cstring> // for std::memcpy
#include <stdexcept>

#include <QThread>
#include <QApplication> // qApp
#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLShaderProgram>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget

#include "Engine/Lut.h" // Color
#include "Engine/OSGLFunctions.h"
#include "Engine/Settings.h"
#include "Engine/Texture.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h" // appFont
#include "Gui/Menu.h"
#include "Gui/ViewerTab.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif
#ifndef M_PI_4
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4           */
#endif



NATRON_NAMESPACE_ENTER

/*This class is the the core of the viewer : what displays images, overlays, etc...
   Everything related to OpenGL will (almost always) be in this class */
ViewerGL::Implementation::Implementation(ViewerGL* this_,
                                         ViewerTab* parent)
    : _this(this_)
    , pboIds()
    , vboVerticesId(0)
    , vboTexturesId(0)
    , iboTriangleStripId(0)
    , displayTextures()
    , partialUpdateTextures()
    , infoViewer()
    , viewerTab(parent)
    , zoomOrPannedSinceLastFit(false)
    , oldClick()
    , displayingImageLut(eViewerColorSpaceSRGB)
    , ms(eMouseStateUndefined)
    , textRenderingColor(200, 200, 200, 255)
    , displayWindowOverlayColor(125, 125, 125, 255)
    , rodOverlayColor(100, 100, 100, 255)
    , textFont(appFont, appFontSize)
    , updatingTexture(false)
    , clearColor(0, 0, 0, 255)
    , persistentMessages()
    , persistentMessageType(0)
    , displayPersistentMessage(false)
    , textRenderer()
    , lastMousePosition()
    , lastDragStartPos()
    , hasMovedSincePress(false)
    , currentViewerInfo_btmLeftBBOXoverlay()
    , currentViewerInfo_topRightBBOXoverlay()
    , currentViewerInfo_resolutionOverlay()
    , pickerState(ePickerStateInactive)
    , lastPickerPos()
    , zoomCtx(0.01, 1024.)   // protected by mutex
    , selectionRectangle()
    , checkerboardTextureID(0)
    , checkerboardTileSize(0)
    , savedTexture(0)
    , prevBoundTexture(0)
    , sizeH()
    , pointerTypeOnPress(ePenTypeLMB)
    , pressureOnPress(1.)
    , pressureOnRelease(1.)
    , wheelDeltaSeekFrame(0)
    , isUpdatingTexture(false)
    , renderOnPenUp(false)
    , updateViewerPboIndex(0)
{
    infoViewer[0] = 0;
    infoViewer[1] = 0;

    assert( qApp && qApp->thread() == QThread::currentThread() );
    //menu->setFont( QFont(appFont,appFontSize) );

    //        QDesktopWidget* desktop = QApplication::desktop();
    //        QRect r = desktop->screenGeometry();
    //        sizeH = r.size();
    sizeH.setWidth(10000);
    sizeH.setHeight(10000);

    glContextWrapper = boost::make_shared<GuiGLContext>(this_);
}

ViewerGL::Implementation::~Implementation()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _this->makeCurrent();


    for (int i = 0; i < 2; ++i) {
        displayTextures[i].texture.reset();
    }
    partialUpdateTextures.clear();

    if ( appPTR && appPTR->isOpenGLLoaded() ) {
        glCheckError(GL_GPU);
        for (U32 i = 0; i < this->pboIds.size(); ++i) {
            GL_GPU::DeleteBuffers(1, &this->pboIds[i]);
        }
        glCheckError(GL_GPU);
        GL_GPU::DeleteBuffers(1, &this->vboVerticesId);
        GL_GPU::DeleteBuffers(1, &this->vboTexturesId);
        GL_GPU::DeleteBuffers(1, &this->iboTriangleStripId);
        glCheckError(GL_GPU);
        GL_GPU::DeleteTextures(1, &this->checkerboardTextureID);
    }
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
    0, 4, 1, 5, 2, 6, 3, 7,
    7, 4,
    4, 8, 5, 9, 6, 10, 7, 11,
    11, 8,
    8, 12, 9, 13, 10, 14, 11, 15
};

/*
   ASCII art of the vertices used to render.
   The actual texture seen on the viewport is the rect (5,9,10,6).
   We draw  3*6 strips
 */

// 0___1___2___3
// |  /|  /|  /|
// | / | / | / |
// |/  |/  |/  |
// 4---5---6----7
// |  /|  /|  /|
// | / | / | / |
// |/  |/  |/  |
// 8---9--10--11
// |  /|  /|  /|
// | / | / | / |
// |/  |/  |/  |
// 12--13--14--15
void
ViewerGL::Implementation::drawRenderingVAO(unsigned int mipMapLevel,
                                           int textureIndex,
                                           ViewerGL::DrawPolygonModeEnum polygonMode,
                                           bool background)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _this->context() );


    ///the texture rectangle in image coordinates. The values in it are multiples of tile size.
    ///
    const RectI &textureBounds = this->displayTextures[textureIndex].texture->getBounds();

    ///This is the coordinates in the image being rendered where datas are valid, this is in pixel coordinates
    ///at the time we initialize it but we will convert it later to canonical coordinates. See 1)
    const double par = this->displayTextures[textureIndex].pixelAspectRatio;

    RectD canonicalRoIRoundedToTileSize;
    textureBounds.toCanonical_noClipping(mipMapLevel, par /*, rod*/, &canonicalRoIRoundedToTileSize);

    ///the RoD of the image in canonical coords.
    RectD rod = _this->getRoD(textureIndex);

    ViewerNodePtr internalNode = _this->getViewerTab()->getInternalNode();
    bool clipToDisplayWindow = internalNode->isClipToFormatEnabled();

    RectD rectClippedToRoI(canonicalRoIRoundedToTileSize);
    rectClippedToRoI.intersect(rod, &rectClippedToRoI);


    if (clipToDisplayWindow) {
        rod.intersect(this->displayTextures[textureIndex].format, &rod);
        rectClippedToRoI.intersect(this->displayTextures[textureIndex].format, &rectClippedToRoI);
    }




    //if user RoI is enabled, clip the rod to that roi
    bool userRoiEnabled = internalNode->isUserRoIEnabled();



    ////The texture real size (r.w,r.h) might be slightly bigger than the actual
    ////pixel coordinates bounds r.x1,r.x2 r.y1 r.y2 because we clipped these bounds against the bounds
    ////in the ViewerInstance::renderViewer function. That means we need to draw actually only the part of
    ////the texture that contains the bounds.
    ////Notice that r.w and r.h are scaled to the closest Po2 of the current scaling factor, so we need to scale it up
    ////So it is in the same coordinates as the bounds.
    ///Edit: we no longer divide by the closestPo2 since the viewer now computes images at lower resolution by itself, the drawing
    ///doesn't need to be scaled.

    if (userRoiEnabled) {
        RectD userRoI = internalNode->getUserRoI();
        //if the userRoI isn't intersecting the rod, just don't render anything
        if ( !rod.intersect(userRoI, &rod) ) {
            return;
        }

        rectClippedToRoI.intersect(rod, &rectClippedToRoI);
        //clipTexCoords<RectD>(canonicalTexRect,rectClippedToRoI,texBottom,texTop,texLeft,texRight);
    }

    if (polygonMode != eDrawPolygonModeWhole) {
        /// draw only  the plane defined by the wipe handle
        QPolygonF polygonPoints, polygonTexCoords;
        RectD floatRectClippedToRoI;
        floatRectClippedToRoI.x1 = rectClippedToRoI.x1;
        floatRectClippedToRoI.y1 = rectClippedToRoI.y1;
        floatRectClippedToRoI.x2 = rectClippedToRoI.x2;
        floatRectClippedToRoI.y2 = rectClippedToRoI.y2;
        Implementation::WipePolygonEnum polyType = this->getWipePolygon(floatRectClippedToRoI, polygonMode == eDrawPolygonModeWipeRight, &polygonPoints);

        if (polyType == Implementation::eWipePolygonEmpty) {
            ///don't draw anything
            return;
        } else if (polyType == Implementation::eWipePolygonPartial) {
            this->getPolygonTextureCoordinates(polygonPoints, canonicalRoIRoundedToTileSize, polygonTexCoords);

            assert(displayTextures[textureIndex].texture);
            GL_GPU::ActiveTexture(GL_TEXTURE0);
            GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&prevBoundTexture);
            GL_GPU::BindTexture( GL_TEXTURE_2D, displayTextures[textureIndex].texture->getTexID() );

            GL_GPU::Begin(GL_POLYGON);
            for (int i = 0; i < polygonTexCoords.size(); ++i) {
                const QPointF & tCoord = polygonTexCoords[i];
                const QPointF & vCoord = polygonPoints[i];
                GL_GPU::TexCoord2d( tCoord.x(), tCoord.y() );
                GL_GPU::Vertex2d( vCoord.x(), vCoord.y() );
            }
            GL_GPU::End();

            GL_GPU::BindTexture( GL_TEXTURE_2D, prevBoundTexture);

        } else {
            ///draw the all polygon as usual
            polygonMode = eDrawPolygonModeWhole;
        }
    }

    if (polygonMode == eDrawPolygonModeWhole) {
        const double pixelCenterOffset = 0.5;
        // draw vertices at the center of the first and last pixel in the texture, with the same texture coordinates
        rectClippedToRoI.x1 += pixelCenterOffset * par;
        rectClippedToRoI.x2 -= pixelCenterOffset * par;
        rectClippedToRoI.y1 += pixelCenterOffset;
        rectClippedToRoI.y2 -= pixelCenterOffset;
        ///Vertices are in canonical coords
        GLfloat vertices[32] = {
            (GLfloat)rod.left(),                                (GLfloat)rod.top(),    //0
            (GLfloat)(rectClippedToRoI.x1 + pixelCenterOffset), (GLfloat)rod.top(),          //1
            (GLfloat)(rectClippedToRoI.x2 - pixelCenterOffset), (GLfloat)rod.top(),    //2
            (GLfloat)rod.right(),                               (GLfloat)rod.top(),   //3
            (GLfloat)rod.left(),                                (GLfloat)(rectClippedToRoI.y2 - pixelCenterOffset), //4
            (GLfloat)rectClippedToRoI.x1,                       (GLfloat)rectClippedToRoI.y2,       //5
            (GLfloat)rectClippedToRoI.x2,                       (GLfloat)rectClippedToRoI.y2, //6
            (GLfloat)rod.right(),                               (GLfloat)rectClippedToRoI.y2, //7
            (GLfloat)rod.left(),                                (GLfloat)rectClippedToRoI.y1,        //8
            (GLfloat)rectClippedToRoI.x1,                       (GLfloat)rectClippedToRoI.y1,             //9
            (GLfloat)rectClippedToRoI.x2,                       (GLfloat)rectClippedToRoI.y1,       //10
            (GLfloat)rod.right(),                               (GLfloat)rectClippedToRoI.y1,       //11
            (GLfloat)rod.left(),                                (GLfloat)rod.bottom(), //12
            (GLfloat)rectClippedToRoI.x1,                       (GLfloat)rod.bottom(),       //13
            (GLfloat)rectClippedToRoI.x2,                       (GLfloat)rod.bottom(), //14
            (GLfloat)rod.right(),                               (GLfloat)rod.bottom() //15
        };


        GLfloat texBottom = (GLfloat)(rectClippedToRoI.y1 - canonicalRoIRoundedToTileSize.y1)  / canonicalRoIRoundedToTileSize.height();
        GLfloat texTop = (GLfloat)(rectClippedToRoI.y2 - canonicalRoIRoundedToTileSize.y1)  / canonicalRoIRoundedToTileSize.height();
        GLfloat texLeft = (GLfloat)(rectClippedToRoI.x1 - canonicalRoIRoundedToTileSize.x1)  / canonicalRoIRoundedToTileSize.width();
        GLfloat texRight = (GLfloat)(rectClippedToRoI.x2 - canonicalRoIRoundedToTileSize.x1)  / canonicalRoIRoundedToTileSize.width();
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


        if ( background && internalNode->isCheckerboardEnabled() && (polygonMode != eDrawPolygonModeWipeRight) ) {
            bool isblend = GL_GPU::IsEnabled(GL_BLEND);
            if (isblend) {
                GL_GPU::Disable(GL_BLEND);
            }
            this->drawCheckerboardTexture(rod);
            if (isblend) {
                GL_GPU::Enable(GL_BLEND);
            }
        }

        assert(displayTextures[textureIndex].texture);
        GL_GPU::ActiveTexture(GL_TEXTURE0);
        GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&prevBoundTexture);
        GL_GPU::BindTexture( GL_TEXTURE_2D, displayTextures[textureIndex].texture->getTexID() );
        glCheckError(GL_GPU);

        GL_GPU::BindBuffer(GL_ARRAY_BUFFER, this->vboVerticesId);
        GL_GPU::BufferSubData(GL_ARRAY_BUFFER, 0, 32 * sizeof(GLfloat), vertices);
        GL_GPU::EnableClientState(GL_VERTEX_ARRAY);
        GL_GPU::VertexPointer(2, GL_FLOAT, 0, 0);

        GL_GPU::BindBuffer(GL_ARRAY_BUFFER, this->vboTexturesId);
        GL_GPU::BufferSubData(GL_ARRAY_BUFFER, 0, 32 * sizeof(GLfloat), renderingTextureCoordinates);
        GL_GPU::ClientActiveTexture(GL_TEXTURE0);
        GL_GPU::EnableClientState(GL_TEXTURE_COORD_ARRAY);
        GL_GPU::TexCoordPointer(2, GL_FLOAT, 0, 0);

        GL_GPU::DisableClientState(GL_COLOR_ARRAY);

        GL_GPU::BindBuffer(GL_ARRAY_BUFFER, 0);

        GL_GPU::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->iboTriangleStripId);
        GL_GPU::DrawElements(GL_TRIANGLE_STRIP, 28, GL_UNSIGNED_BYTE, 0);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        GL_GPU::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_GPU::DisableClientState(GL_VERTEX_ARRAY);
        GL_GPU::DisableClientState(GL_TEXTURE_COORD_ARRAY);
        GL_GPU::BindTexture( GL_TEXTURE_2D, prevBoundTexture);
        glCheckError(GL_GPU);

    }
} // drawRenderingVAO

void
ViewerGL::Implementation::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _this->makeCurrent();
    initAndCheckGlExtensions();

    int format, internalFormat, glType;
    Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
    displayTextures[0].texture.reset( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, eImageBitDepthByte, format, internalFormat, glType, true) );
    displayTextures[1].texture.reset( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, eImageBitDepthByte, format, internalFormat, glType, true) );


    GL_GPU::GenBuffers(1, &this->vboVerticesId);
    GL_GPU::GenBuffers(1, &this->vboTexturesId);
    GL_GPU::GenBuffers(1, &this->iboTriangleStripId);

    GL_GPU::BindBuffer(GL_ARRAY_BUFFER, this->vboTexturesId);
    GL_GPU::BufferData(GL_ARRAY_BUFFER, 32 * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);

    GL_GPU::BindBuffer(GL_ARRAY_BUFFER, this->vboVerticesId);
    GL_GPU::BufferData(GL_ARRAY_BUFFER, 32 * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    GL_GPU::BindBuffer(GL_ARRAY_BUFFER, 0);

    GL_GPU::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->iboTriangleStripId);
    GL_GPU::BufferData(GL_ELEMENT_ARRAY_BUFFER, 28 * sizeof(GLubyte), triangleStrip, GL_STATIC_DRAW);

    GL_GPU::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError(GL_GPU);

    initializeCheckerboardTexture(true);


    glCheckError(GL_GPU);
}

bool
ViewerGL::Implementation::initAndCheckGlExtensions()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _this->context() );
    assert( QGLShaderProgram::hasOpenGLShaderPrograms( _this->context() ) );

    return true;
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
                                         bool rightPlane,
                                         QPolygonF * polygonPoints) const
{
    ///Compute a second point on the plane separator line
    ///we don't really care how far it is from the center point, it just has to be on the line
    ViewerNodePtr node = _this->getViewerTab()->getInternalNode();
    QPointF center = node->getWipeCenter();
    double angle = node->getWipeAngle();


    ///extrapolate the line to the maximum size of the RoD so we're sure the line
    ///intersection algorithm works
    double maxSize = std::max(texRectClipped.x2 - texRectClipped.x1, texRectClipped.y2 - texRectClipped.y1) * 10000.;
    double xmax, ymax;

    xmax = std::cos(angle + M_PI_2) * maxSize;
    ymax = std::sin(angle + M_PI_2) * maxSize;


    // first, compute whether the whole rectangle is on one side of the wipe
    const QPointF firstPoint ( center.x() + (rightPlane ? xmax : -xmax), center.y() + (rightPlane ? ymax : -ymax) );
    const QPointF secondPoint( center.x() + (rightPlane ? -xmax : xmax), center.y() + (rightPlane ? -ymax : ymax) );
    double crossProd11  = ( ( secondPoint.x() - center.x() ) * ( texRectClipped.y1 - center.y() )
                            - ( secondPoint.y() - center.y() ) * ( texRectClipped.x1 - center.x() ) );
    double crossProd12  = ( ( secondPoint.x() - center.x() ) * ( texRectClipped.y2 - center.y() )
                            - ( secondPoint.y() - center.y() ) * ( texRectClipped.x1 - center.x() ) );
    double crossProd21  = ( ( secondPoint.x() - center.x() ) * ( texRectClipped.y1 - center.y() )
                            - ( secondPoint.y() - center.y() ) * ( texRectClipped.x2 - center.x() ) );
    double crossProd22  = ( ( secondPoint.x() - center.x() ) * ( texRectClipped.y2 - center.y() )
                            - ( secondPoint.y() - center.y() ) * ( texRectClipped.x2 - center.x() ) );

    polygonPoints->clear();

    // if all cross products have the same sign, the rectangle is on one side
    if ( (crossProd11 >= 0) && (crossProd12 >= 0) && (crossProd21 >= 0) && (crossProd22 >= 0) ) {
        return ViewerGL::Implementation::eWipePolygonFull;
    }
    if ( (crossProd11 <= 0) && (crossProd12 <= 0) && (crossProd21 <= 0) && (crossProd22 <= 0) ) {
        return ViewerGL::Implementation::eWipePolygonEmpty;
    }

    // now go through all four corners:
    // - if the cross product is positive, the corner must be inserted
    // - if the cross-product changes sign then the intersection must be inserted
    const QLineF inter(firstPoint, secondPoint);
    if (crossProd11 >= 0) {
        *polygonPoints << QPointF(texRectClipped.x1, texRectClipped.y1);
    }
    if (crossProd11 * crossProd21 < 0) {
        QLineF e(texRectClipped.x1, texRectClipped.y1, texRectClipped.x2, texRectClipped.y1);
        QPointF p;
        QLineF::IntersectType t = inter.intersect(e, &p);
        if (t == QLineF::BoundedIntersection) {
            *polygonPoints << p;
        }
    }
    if (crossProd21 >= 0) {
        *polygonPoints << QPointF(texRectClipped.x2, texRectClipped.y1);
    }
    if (crossProd21 * crossProd22 < 0) {
        QLineF e(texRectClipped.x2, texRectClipped.y1, texRectClipped.x2, texRectClipped.y2);
        QPointF p;
        QLineF::IntersectType t = inter.intersect(e, &p);
        if (t == QLineF::BoundedIntersection) {
            *polygonPoints << p;
        }
    }
    if (crossProd22 >= 0) {
        *polygonPoints << QPointF(texRectClipped.x2, texRectClipped.y2);
    }
    if (crossProd22 * crossProd12 < 0) {
        QLineF e(texRectClipped.x2, texRectClipped.y2, texRectClipped.x1, texRectClipped.y2);
        QPointF p;
        QLineF::IntersectType t = inter.intersect(e, &p);
        if (t == QLineF::BoundedIntersection) {
            *polygonPoints << p;
        }
    }
    if (crossProd12 >= 0) {
        *polygonPoints << QPointF(texRectClipped.x1, texRectClipped.y2);
    }
    if (crossProd12 * crossProd11 < 0) {
        QLineF e(texRectClipped.x1, texRectClipped.y2, texRectClipped.x1, texRectClipped.y1);
        QPointF p;
        QLineF::IntersectType t = inter.intersect(e, &p);
        if (t == QLineF::BoundedIntersection) {
            *polygonPoints << p;
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
            GL_GPU::Enable(GL_BLEND);
        }
        switch (premult) {
        case eImagePremultiplicationPremultiplied:
            GL_GPU::BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case eImagePremultiplicationUnPremultiplied:
            GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case eImagePremultiplicationOpaque:
            break;
        }
    }

    ~BlendSetter()
    {
        if (didBlend) {
            GL_GPU::Disable(GL_BLEND);
        }
    }
};

void
ViewerGL::Implementation::drawSelectionRectangle()
{
    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::Color4f(0.5, 0.8, 1., 0.4);
        QPointF btmRight = selectionRectangle.bottomRight();
        QPointF topLeft = selectionRectangle.topLeft();

        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::End();


        GL_GPU::LineWidth(1.5);

        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::End();

        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
}

void
ViewerGL::Implementation::drawCheckerboardTexture(const RectD& rod)
{
    ///We divide by 2 the tiles count because one texture is 4 tiles actually
    QPointF topLeft, btmRight;
    double screenW, screenH;
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

    GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib<GL_GPU> a(GL_SCISSOR_BIT | GL_ENABLE_BIT);

        GL_GPU::Enable(GL_SCISSOR_TEST);
        GL_GPU::Scissor( rodBtmLeft.x(), screenH - rodBtmLeft.y(), rodTopRight.x() - rodBtmLeft.x(), rodBtmLeft.y() - rodTopRight.y() );

        GL_GPU::Enable(GL_TEXTURE_2D);
        GL_GPU::BindTexture(GL_TEXTURE_2D, checkerboardTextureID);
        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::TexCoord2d(0., 0.);
        GL_GPU::Vertex2d( topLeft.x(), btmRight.y() );
        GL_GPU::TexCoord2d(0., yTilesCountF);
        GL_GPU::Vertex2d( topLeft.x(), topLeft.y() );
        GL_GPU::TexCoord2d(xTilesCountF, yTilesCountF);
        GL_GPU::Vertex2d( btmRight.x(), topLeft.y() );
        GL_GPU::TexCoord2d(xTilesCountF, 0.);
        GL_GPU::Vertex2d( btmRight.x(), btmRight.y() );
        GL_GPU::End();

        //glDisable(GL_SCISSOR_TEST);
    } // GLProtectAttrib a(GL_SCISSOR_BIT | GL_ENABLE_BIT);
    GL_GPU::BindTexture(GL_TEXTURE_2D, savedTexture);
    glCheckError(GL_GPU);
}

void
ViewerGL::Implementation::drawCheckerboardTexture(const QPolygonF& polygon)
{
    ///We divide by 2 the tiles count because one texture is 4 tiles actually
    QPointF topLeft, btmRight;
    double screenW, screenH;
    {
        QMutexLocker l(&zoomCtxMutex);
        topLeft = zoomCtx.toZoomCoordinates(0, 0);
        screenW = zoomCtx.screenWidth();
        screenH = zoomCtx.screenHeight();
        btmRight = zoomCtx.toZoomCoordinates(screenW - 1, screenH - 1);
    }
    double xTilesCountF = screenW / (checkerboardTileSize * 4); //< 4 because the texture contains 4 tiles
    double yTilesCountF = screenH / (checkerboardTileSize * 4);
    GLuint savedTexture;

    GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib<GL_GPU> a(GL_ENABLE_BIT);

        GL_GPU::Enable(GL_TEXTURE_2D);
        GL_GPU::BindTexture(GL_TEXTURE_2D, checkerboardTextureID);
        GL_GPU::Begin(GL_POLYGON);
        for (QPolygonF::const_iterator it = polygon.begin();
             it != polygon.end();
             ++it) {
            GL_GPU::TexCoord2d( xTilesCountF * ( it->x() - topLeft.x() )  / ( btmRight.x() - topLeft.x() ),
                          yTilesCountF * ( it->y() - btmRight.y() ) / ( topLeft.y() - btmRight.y() ) );
            GL_GPU::Vertex2d( it->x(), it->y() );
        }
        GL_GPU::End();


        //glDisable(GL_SCISSOR_TEST);
    } // GLProtectAttrib a(GL_SCISSOR_BIT | GL_ENABLE_BIT);
    GL_GPU::BindTexture(GL_TEXTURE_2D, savedTexture);
    glCheckError(GL_GPU);
}

void
ViewerGL::Implementation::initializeCheckerboardTexture(bool mustCreateTexture)
{
    if (mustCreateTexture) {
        GL_GPU::GenTextures(1, &checkerboardTextureID);
    }
    GLuint savedTexture;
    GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib<GL_GPU> a(GL_ENABLE_BIT);

        GL_GPU::Enable(GL_TEXTURE_2D);
        GL_GPU::BindTexture (GL_TEXTURE_2D, checkerboardTextureID);

        GL_GPU::TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_GPU::TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GL_GPU::TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GL_GPU::TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
        std::memcpy(&checkerboardTexture[8], &checkerboardTexture[4], sizeof(unsigned char) * 4);
        std::memcpy(&checkerboardTexture[12], &checkerboardTexture[0], sizeof(unsigned char) * 4);

        GL_GPU::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, (void*)checkerboardTexture);
    } // GLProtectAttrib a(GL_ENABLE_BIT);
    GL_GPU::BindTexture(GL_TEXTURE_2D, savedTexture);

    checkerboardTileSize = appPTR->getCurrentSettings()->getCheckerboardTileSize();
}


void
ViewerGL::Implementation::refreshSelectionRectangle(const QPointF & pos)
{
    double xmin = std::min( pos.x(), lastDragStartPos.x() );
    double xmax = std::max( pos.x(), lastDragStartPos.x() );
    double ymin = std::min( pos.y(), lastDragStartPos.y() );
    double ymax = std::max( pos.y(), lastDragStartPos.y() );

    selectionRectangle.setRect(xmin, ymin, xmax - xmin, ymax - ymin);
}


NATRON_NAMESPACE_EXIT

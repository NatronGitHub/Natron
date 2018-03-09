/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "ViewerGL.h"
#include "ViewerGLPrivate.h"

#include <cassert>
#include <algorithm> // min, max
#include <cstring> // for std::memcpy, std::memset, std::strcmp, std::strchr, strlen
#include <stdexcept>

#include <QThread>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QApplication> // qApp
#else
#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QApplication> // qApp
#endif

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLShaderProgram>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
#include <QTreeWidget>
#include <QTabBar>

#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/NodeGuiI.h"
#include "Engine/Image.h"
#include "Engine/ImagePrivate.h"
#include "Engine/Project.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/KnobTypes.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RenderEngine.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h" // for gettimeofday
#include "Engine/Texture.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h" // kShortcutGroupViewer ...
#include "Gui/Gui.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // isMouseShortcut
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h" // buttonDownIsLeft...
#include "Gui/Histogram.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGuiParametric.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/Shaders.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"



#define PERSISTENT_MESSAGE_LEFT_OFFSET_PIXELS 20

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif
#ifndef M_PI_4
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4           */
#endif

#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif


/*This class is the the core of the viewer : what displays images, overlays, etc...
   Everything related to OpenGL will (almost always) be in this class */

NATRON_NAMESPACE_ENTER


ViewerGL::ViewerGL(ViewerTab* parent,
                   const QGLWidget* shareWidget)
    : QGLWidget(parent, shareWidget)
    , _imp( new Implementation(this, parent) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    QObject::connect( appPTR, SIGNAL(checkerboardSettingsChanged()), this, SLOT(onCheckerboardSettingsChanged()) );
    QObject::connect( this, SIGNAL(mustCallUpdateOnMainThread()), this, SLOT(update()));
    QObject::connect( this, SIGNAL(mustCallUpdateGLOnMainThread()), this, SLOT(updateGL()));
}

ViewerGL::~ViewerGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

QSize
ViewerGL::sizeHint() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->sizeH;
}

const QFont &
ViewerGL::textFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->textFont;
}

void
ViewerGL::setTextFont(const QFont & f)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->textFont = f;
}

/**
 *@returns Returns true if the viewer is displaying something.
 **/
bool
ViewerGL::displayingImage() const
{
    // always running in the main thread
    QMutexLocker k(&_imp->displayDataMutex);
    return _imp->displayTextures[0].isVisible || _imp->displayTextures[1].isVisible;
}

OSGLContextPtr
ViewerGL::getOpenGLViewerContext() const
{
    return _imp->glContextWrapper;
}

void
ViewerGL::resizeGL(int w,
                   int h)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( (h == 0) || (w == 0) ) { // prevent division by 0
        return;
    }
    glCheckError(GL_GPU);
    assert( w == width() && h == height() ); // if this crashes here, then the viewport size has to be stored to compute glShadow
    GL_GPU::Viewport (0, 0, w, h);
    bool zoomSinceLastFit;
    int oldWidth, oldHeight;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        oldWidth = _imp->zoomCtx.screenWidth();
        oldHeight = _imp->zoomCtx.screenHeight();
        _imp->zoomCtx.setScreenSize(w, h, /*alignTop=*/ true, /*alignRight=*/ false);
        zoomSinceLastFit = _imp->zoomOrPannedSinceLastFit;
    }
    glCheckError(GL_GPU);
    _imp->ms = eMouseStateUndefined;
    assert(_imp->viewerTab);
    ViewerNodePtr viewerNode = _imp->viewerTab->getInternalNode();
    if (!viewerNode) {
        return;
    }


    bool isLoadingProject = _imp->viewerTab->getGui() && _imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject();
    bool canResize = _imp->viewerTab->getGui() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject() &&  !_imp->viewerTab->getGui()->getApp()->getActiveRotoDrawingStroke();

    if (!zoomSinceLastFit && canResize) {
        fitImageToFormat();
    }
    if ( viewerNode->getUiContext() && !isLoadingProject  &&
         ( ( oldWidth != w) || ( oldHeight != h) ) ) {
        viewerNode->getNode()->getRenderEngine()->renderCurrentFrame();

        if ( !_imp->persistentMessages.empty() ) {
            updatePersistentMessageToWidth(w - 20);
        } else {
            update();
        }
    }
}

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
ViewerGL::glDraw()
{
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(_imp->glContextWrapper);
    locker->attach();
    QGLWidget::glDraw();
}

void
ViewerGL::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(QGLContext::currentContext() == context());
    //if app is closing, just return
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }
    glCheckError(GL_GPU);

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
        clearColorBuffer( _imp->clearColor.redF(), _imp->clearColor.greenF(), _imp->clearColor.blueF(), _imp->clearColor.alphaF() );
        glCheckError(GL_GPU);

        return;
    }

    {
        //GLProtectAttrib a(GL_TRANSFORM_BIT); // GL_MODELVIEW is active by default

        // Note: the OFX spec says that the GL_MODELVIEW should be the identity matrix
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectOverlays
        // However,
        // - Nuke uses a different matrix (RoD.width is the width of the RoD of the displayed image)
        // - Nuke transforms the interacts using the modelview if there are Transform nodes between the viewer and the interact.


        GL_GPU::MatrixMode(GL_PROJECTION);
        GL_GPU::LoadIdentity();
        GL_GPU::Ortho(zoomLeft, zoomRight, zoomBottom, zoomTop, -1, 1);
        _imp->glShadow.setX( (zoomRight - zoomLeft) / width() );
        _imp->glShadow.setY( (zoomTop - zoomBottom) / height() );
        //glScalef(RoD.width, RoD.width, 1.0); // for compatibility with Nuke
        //glTranslatef(1, 1, 0);     // for compatibility with Nuke
        GL_GPU::MatrixMode(GL_MODELVIEW);
        GL_GPU::LoadIdentity();
        //glTranslatef(-1, -1, 0);        // for compatibility with Nuke
        //glScalef(1/RoD.width, 1./RoD.width, 1.0); // for compatibility with Nuke

        glCheckError(GL_GPU);

        ViewerNodePtr viewerNode = _imp->viewerTab->getInternalNode();

        NodePtr aInputNode,bInputNode;


        GLuint savedTexture;
        GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
        {
            GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

            clearColorBuffer( _imp->clearColor.redF(), _imp->clearColor.greenF(), _imp->clearColor.blueF(), _imp->clearColor.alphaF() );
            glCheckErrorIgnoreOSXBug(GL_GPU);


            if (!viewerNode) {
                return;
            }
            // don't even bind the shader on 8-bits gamma-compressed textures
            ViewerCompositingOperatorEnum compOperator = viewerNode->getCurrentOperator();

            aInputNode = viewerNode->getCurrentAInput();
            bInputNode = viewerNode->getCurrentBInput();

            bool drawTexture[2];
            {
                QMutexLocker k(&_imp->displayDataMutex);
                drawTexture[0] = _imp->displayTextures[0].isVisible;
                drawTexture[1] = _imp->displayTextures[1].isVisible && compOperator != eViewerCompositingOperatorNone;
            }
            if ( (aInputNode == bInputNode) &&
                (compOperator != eViewerCompositingOperatorWipeMinus) &&
                (compOperator != eViewerCompositingOperatorStackMinus) ) {
                drawTexture[1] = false;
            }

            double wipeMix = viewerNode->getWipeAmount();

            bool stack = (compOperator == eViewerCompositingOperatorStackUnder ||
                          compOperator == eViewerCompositingOperatorStackOver ||
                          compOperator == eViewerCompositingOperatorStackMinus ||
                          compOperator == eViewerCompositingOperatorStackOnionSkin ||
                          false);


            GL_GPU::Enable (GL_TEXTURE_2D);
            GL_GPU::Color4d(1., 1., 1., 1.);
            GL_GPU::BlendColor(1, 1, 1, wipeMix);

            bool checkerboard = viewerNode->isCheckerboardEnabled();
            if (checkerboard) {
                // draw checkerboard texture, but only on the left side if in wipe mode
                RectD canonicalFormat;
                {
                    QMutexLocker k(&_imp->displayDataMutex);
                    canonicalFormat = _imp->displayTextures[0].format;
                }
                if (compOperator == eViewerCompositingOperatorNone) {
                    _imp->drawCheckerboardTexture(canonicalFormat);
                } else if ( operatorIsWipe(compOperator) ) {
                    QPolygonF polygonPoints;
                    Implementation::WipePolygonEnum t = _imp->getWipePolygon(canonicalFormat,
                                                                             false,
                                                                             &polygonPoints);
                    if (t == Implementation::eWipePolygonFull) {
                        _imp->drawCheckerboardTexture(canonicalFormat);
                    } else if (t == Implementation::eWipePolygonPartial) {
                        _imp->drawCheckerboardTexture(polygonPoints);
                    }
                }
            }


            ///Depending on the premultiplication of the input image we use a different blending func
            ImagePremultiplicationEnum premultA;
            unsigned int mipMapLevels[2];
            double par;
            {
                QMutexLocker k(&_imp->displayDataMutex);
                premultA = _imp->displayTextures[0].premult;
                par = _imp->displayTextures[0].pixelAspectRatio;
                mipMapLevels[0] = _imp->displayTextures[0].mipMapLevel;
                mipMapLevels[1] = _imp->displayTextures[1].mipMapLevel;
            }

            // Left side of the wipe is displayed as Opaque if there is no checkerboard.
            // That way, unpremultiplied images can easily be displayed, even if their alpha is zero.
            // We do not "unpremult" premultiplied RGB for displaying it, because it is the usual way
            // to visualize masks: areas with alpha=0 appear as black.
            switch (compOperator) {
            case eViewerCompositingOperatorNone: {
                if (drawTexture[0]) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, eDrawPolygonModeWhole, true);
                }
                break;
            }
            case eViewerCompositingOperatorWipeUnder:
            case eViewerCompositingOperatorStackUnder: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }
                if (drawTexture[1]) {
                    GL_GPU::Enable(GL_BLEND);
                    GL_GPU::BlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(mipMapLevels[1], 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::Disable(GL_BLEND);
                }

                break;
            }
            case eViewerCompositingOperatorWipeOver:
            case eViewerCompositingOperatorStackOver: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[1]) {
                    GL_GPU::Enable(GL_BLEND);
                    GL_GPU::BlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(mipMapLevels[1], 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::Disable(GL_BLEND);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }

                break;
            }
            case eViewerCompositingOperatorWipeMinus:
            case eViewerCompositingOperatorStackMinus: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }
                if (drawTexture[1]) {
                    GL_GPU::Enable(GL_BLEND);
                    GL_GPU::BlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                    GL_GPU::BlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                    _imp->drawRenderingVAO(mipMapLevels[1], 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::Disable(GL_BLEND);
                }
                break;
            }
            case eViewerCompositingOperatorWipeOnionSkin:
            case eViewerCompositingOperatorStackOnionSkin: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(mipMapLevels[0], 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }
                if (drawTexture[1]) {
                    GL_GPU::Enable(GL_BLEND);
                    GL_GPU::BlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                    //glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                    _imp->drawRenderingVAO(mipMapLevels[1], 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::Disable(GL_BLEND);
                }
                break;
            }
            } // switch

            {
                QMutexLocker k(&_imp->displayDataMutex);
                for (std::size_t i = 0; i < _imp->partialUpdateTextures.size(); ++i) {
                    const RectI &texRect = _imp->partialUpdateTextures[i].texture->getBounds();
                    RectD canonicalTexRect;
                    texRect.toCanonical_noClipping(_imp->partialUpdateTextures[i].mipMapLevel, par /*, rod*/, &canonicalTexRect);

                    GL_GPU::ActiveTexture(GL_TEXTURE0);
                    GL_GPU::BindTexture( GL_TEXTURE_2D, _imp->partialUpdateTextures[i].texture->getTexID() );
                    GL_GPU::Begin(GL_POLYGON);
                    GL_GPU::TexCoord2d(0, 0); GL_GPU::Vertex2d(canonicalTexRect.x1, canonicalTexRect.y1);
                    GL_GPU::TexCoord2d(0, 1); GL_GPU::Vertex2d(canonicalTexRect.x1, canonicalTexRect.y2);
                    GL_GPU::TexCoord2d(1, 1); GL_GPU::Vertex2d(canonicalTexRect.x2, canonicalTexRect.y2);
                    GL_GPU::TexCoord2d(1, 0); GL_GPU::Vertex2d(canonicalTexRect.x2, canonicalTexRect.y1);
                    GL_GPU::End();
                    GL_GPU::BindTexture(GL_TEXTURE_2D, 0);
                    
                    glCheckError(GL_GPU);
                }
            }
        } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

        ///Unbind render textures for overlays
        GL_GPU::BindTexture(GL_TEXTURE_2D, savedTexture);

        glCheckError(GL_GPU);
        if (viewerNode->isOverlayEnabled()) {
            drawOverlay( getCurrentRenderScale(), aInputNode, bInputNode );
        } else {
            const QFont& f = font();
            QFontMetrics fm(f);
            QPointF pos;
            {
                QMutexLocker k(&_imp->zoomCtxMutex);
                pos = _imp->zoomCtx.toZoomCoordinates( 10, height() - fm.height() );
            }
            renderText(pos.x(), pos.y(), tr("Overlays off"), QColor(200, 0, 0), f);
        }

        if (_imp->ms == eMouseStateSelecting) {
            _imp->drawSelectionRectangle();
        }
        glCheckErrorAssert(GL_GPU);
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
    glCheckError(GL_GPU);
    {
        GLProtectAttrib<GL_GPU> att(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

        GL_GPU::ClearColor(r, g, b, a);
        GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glCheckErrorIgnoreOSXBug(GL_GPU);
}


void
ViewerGL::drawOverlay(unsigned int mipMapLevel,
                      const NodePtr& /*aInput*/,
                      const NodePtr& bInput)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    glCheckError(GL_GPU);


    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        GL_GPU::Disable(GL_BLEND);


        ViewerNodePtr viewerNode = _imp->viewerTab->getInternalNode();
        if (!viewerNode) {
            return;
        }
        ViewerCompositingOperatorEnum compOperator = viewerNode->getCurrentOperator();


        for (int i = 0; i < 2; ++i) {

            if ( (i == 1) && (compOperator == eViewerCompositingOperatorNone) ) {
                break;
            }
            RectD canonicalFormat = getCanonicalFormat(i);

            // Draw format
            {
                QString str;
                {
                    QMutexLocker k(&_imp->displayDataMutex);
                    str = _imp->currentViewerInfo_resolutionOverlay[i];
                }
                renderText(canonicalFormat.right(), canonicalFormat.bottom(), str, _imp->textRenderingColor, _imp->textFont);


                QPoint topRight( canonicalFormat.right(), canonicalFormat.top() );
                QPoint topLeft( canonicalFormat.left(), canonicalFormat.top() );
                QPoint btmLeft( canonicalFormat.left(), canonicalFormat.bottom() );
                QPoint btmRight( canonicalFormat.right(), canonicalFormat.bottom() );

                GL_GPU::Begin(GL_LINES);

                GL_GPU::Color4f( _imp->displayWindowOverlayColor.redF(),
                                  _imp->displayWindowOverlayColor.greenF(),
                                  _imp->displayWindowOverlayColor.blueF(),
                                  _imp->displayWindowOverlayColor.alphaF() );
                GL_GPU::Vertex3f(btmRight.x(), btmRight.y(), 1);
                GL_GPU::Vertex3f(btmLeft.x(), btmLeft.y(), 1);

                GL_GPU::Vertex3f(btmLeft.x(), btmLeft.y(), 1);
                GL_GPU::Vertex3f(topLeft.x(), topLeft.y(), 1);

                GL_GPU::Vertex3f(topLeft.x(), topLeft.y(), 1);
                GL_GPU::Vertex3f(topRight.x(), topRight.y(), 1);

                GL_GPU::Vertex3f(topRight.x(), topRight.y(), 1);
                GL_GPU::Vertex3f(btmRight.x(), btmRight.y(), 1);
                
                GL_GPU::End();
                glCheckErrorIgnoreOSXBug(GL_GPU);
            }

            bool visible;
            {
                QMutexLocker k(&_imp->displayDataMutex);
                visible = _imp->displayTextures[i].isVisible;
            }
            if ( !visible || (!bInput && i == 1)) {
                continue;
            }
            RectD dataW = getRoD(i);


            if (dataW != canonicalFormat) {
                QString topRightStr, btmLeftStr;
                {
                    QMutexLocker k(&_imp->displayDataMutex);
                    topRightStr = _imp->currentViewerInfo_topRightBBOXoverlay[i];
                    btmLeftStr = _imp->currentViewerInfo_btmLeftBBOXoverlay[i];
                }
                renderText(dataW.right(), dataW.top(), topRightStr, _imp->rodOverlayColor, _imp->textFont);
                renderText(dataW.left(), dataW.bottom(), btmLeftStr, _imp->rodOverlayColor, _imp->textFont);
                glCheckError(GL_GPU);

                QPointF topRight2( dataW.right(), dataW.top() );
                QPointF topLeft2( dataW.left(), dataW.top() );
                QPointF btmLeft2( dataW.left(), dataW.bottom() );
                QPointF btmRight2( dataW.right(), dataW.bottom() );
                GL_GPU::LineStipple(2, 0xAAAA);
                GL_GPU::Enable(GL_LINE_STIPPLE);
                GL_GPU::Begin(GL_LINES);
                GL_GPU::Color4f( _imp->rodOverlayColor.redF(),
                           _imp->rodOverlayColor.greenF(),
                           _imp->rodOverlayColor.blueF(),
                           _imp->rodOverlayColor.alphaF() );
                GL_GPU::Vertex3f(btmRight2.x(), btmRight2.y(), 1);
                GL_GPU::Vertex3f(btmLeft2.x(), btmLeft2.y(), 1);

                GL_GPU::Vertex3f(btmLeft2.x(), btmLeft2.y(), 1);
                GL_GPU::Vertex3f(topLeft2.x(), topLeft2.y(), 1);

                GL_GPU::Vertex3f(topLeft2.x(), topLeft2.y(), 1);
                GL_GPU::Vertex3f(topRight2.x(), topRight2.y(), 1);

                GL_GPU::Vertex3f(topRight2.x(), topRight2.y(), 1);
                GL_GPU::Vertex3f(btmRight2.x(), btmRight2.y(), 1);
                GL_GPU::End();
                GL_GPU::Disable(GL_LINE_STIPPLE);
                glCheckErrorIgnoreOSXBug(GL_GPU);
            }
        }


        glCheckError(GL_GPU);
        GL_GPU::Color4f(1., 1., 1., 1.);
        double scale = 1. / (1 << mipMapLevel);

        /*
           Draw the overlays corresponding to the image displayed on the viewer, not the current timeline's time
         */
        TimeValue time = getCurrentlyDisplayedTime();
        _imp->viewerTab->drawOverlays( time, RenderScale(scale) );

        glCheckErrorIgnoreOSXBug(GL_GPU);

        if (_imp->pickerState == ePickerStateRectangle) {
                drawPickerRectangle();
        } else if (_imp->pickerState == ePickerStatePoint) {
                drawPickerPixel();
        }
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError(GL_GPU);

    if (_imp->displayPersistentMessage) {
        drawPersistentMessage();
    }
} // drawOverlay


void
ViewerGL::drawPickerRectangle()
{
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT);

        GL_GPU::Color3f(0.9, 0.7, 0.);
        QPointF topLeft = _imp->pickerRect.topLeft();
        QPointF btmRight = _imp->pickerRect.bottomRight();
        ///base rect
        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f( topLeft.x(), btmRight.y() ); //bottom left
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() ); //top left
        GL_GPU::Vertex2f( btmRight.x(), topLeft.y() ); //top right
        GL_GPU::Vertex2f( btmRight.x(), btmRight.y() ); //bottom right
        GL_GPU::End();
    } // GLProtectAttrib a(GL_CURRENT_BIT);
}

void
ViewerGL::drawPickerPixel()
{
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_POINT_SMOOTH);
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            GL_GPU::PointSize( 1. * _imp->zoomCtx.factor() );
        }

        QPointF pos = _imp->lastPickerPos;
        unsigned int mipMapLevel;
        {
            QMutexLocker k(&_imp->displayDataMutex);
            mipMapLevel = _imp->displayTextures[0].mipMapLevel;
        }

        if (mipMapLevel != 0) {
            pos *= (1 << mipMapLevel);
        }
        GL_GPU::Color3f(0.9, 0.7, 0.);
        GL_GPU::Begin(GL_POINTS);
        GL_GPU::Vertex2d( pos.x(), pos.y() );
        GL_GPU::End();
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT);
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

static QStringList
explode(const QString& str)
{
    QRegExp rx( QString::fromUtf8("(\\ |\\-|\\.|\\/|\\t|\\n)") ); //RegEx for ' ' '/' '.' '-' '\t' '\n'
    QStringList ret;
    int startIndex = 0;

    while (true) {
        int index = str.indexOf(rx, startIndex);

        if (index == -1) {
            ret.push_back( str.mid(startIndex) );

            return ret;
        }

        QString word = str.mid(startIndex, index - startIndex);
        const QChar& nextChar = str[index];

        // Dashes and the likes should stick to the word occuring before it. Whitespace doesn't have to.
        if ( nextChar.isSpace() ) {
            ret.push_back(word);
            ret.push_back(nextChar);
        } else {
            ret.push_back(word + nextChar);
        }

        startIndex = index + 1;
    }

    return ret;
}

static QStringList
wordWrap(const QFontMetrics& fm,
         const QString& str,
         int width)
{
    QStringList words = explode(str);
    int curLineLength = 0;
    QStringList stringL;
    QString curString;

    for (int i = 0; i < words.size(); ++i) {
        QString word = words[i];
        int wordPixels = fm.width(word);

        // If adding the new word to the current line would be too long,
        // then put it on a new line (and split it up if it's too long).
        if (curLineLength + wordPixels > width) {
            // Only move down to a new line if we have text on the current line.
            // Avoids situation where wrapped whitespace causes emptylines in text.
            if (curLineLength > 0) {
                if ( !curString.isEmpty() ) {
                    stringL.push_back(curString);
                    curString.clear();
                }
                //tmp.append('\n');
                curLineLength = 0;
            }

            // If the current word is too long to fit on a line even on it's own then
            // split the word up.
            while (wordPixels > width) {
                curString.clear();
                curString.append( word.mid(0, width - 1) );
                word = word.mid(width - 1);

                if ( !curString.isEmpty() ) {
                    stringL.push_back(curString);
                    curString.clear();
                }
                wordPixels = fm.width(word);
                //tmp.append('\n');
            }

            // Remove leading whitespace from the word so the new line starts flush to the left.
            word = word.trimmed();
        }
        curString.append(word);
        curLineLength += wordPixels;
    }
    if ( !curString.isEmpty() ) {
        stringL.push_back(curString);
    }

    return stringL;
} // wordWrap

NATRON_NAMESPACE_ANONYMOUS_EXIT


void
ViewerGL::drawPersistentMessage()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    QFontMetrics metrics( _imp->textFont );
    int offset =  10;
    double metricsHeightZoomCoord;
    QPointF topLeft, bottomRight, offsetZoomCoord;

    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        topLeft = _imp->zoomCtx.toZoomCoordinates(0, 0);
        bottomRight = _imp->zoomCtx.toZoomCoordinates( _imp->zoomCtx.screenWidth(), _imp->persistentMessages.size() * (metrics.height() + offset) );
        offsetZoomCoord = _imp->zoomCtx.toZoomCoordinates(PERSISTENT_MESSAGE_LEFT_OFFSET_PIXELS, offset);
        metricsHeightZoomCoord = topLeft.y() - _imp->zoomCtx.toZoomCoordinates( 0, metrics.height() ).y();
    }
    offsetZoomCoord.ry() = topLeft.y() - offsetZoomCoord.y();
    QPointF textPos(offsetZoomCoord.x(),  topLeft.y() - (offsetZoomCoord.y() / 2.) - metricsHeightZoomCoord);

    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::Disable(GL_BLEND);

        if (_imp->persistentMessageType == 1) { // error
            GL_GPU::Color4f(0.5, 0., 0., 1.);
        } else { // warning
            GL_GPU::Color4f(0.65, 0.65, 0., 1.);
        }
        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() ); //top left
        GL_GPU::Vertex2f( topLeft.x(), bottomRight.y() ); //bottom left
        GL_GPU::Vertex2f( bottomRight.x(), bottomRight.y() ); //bottom right
        GL_GPU::Vertex2f( bottomRight.x(), topLeft.y() ); //top right
        GL_GPU::End();


        for (int j = 0; j < _imp->persistentMessages.size(); ++j) {
            renderText(textPos.x(), textPos.y(), _imp->persistentMessages.at(j), _imp->textRenderingColor, _imp->textFont);
            textPos.setY( textPos.y() - ( metricsHeightZoomCoord + offsetZoomCoord.y() ) ); /*metrics.height() * 2 * zoomScreenPixelHeight*/
        }
        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
} // drawPersistentMessage

void
ViewerGL::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    appPTR->initializeOpenGLFunctionsOnce();
    if ( !appPTR->isOpenGLLoaded() ) {
        throw std::runtime_error("OpenGL was not loaded");
    }
    _imp->initializeGL();
}

GLuint
ViewerGL::getPboID(int index)
{
    // always running in the main thread
    assert( QGLContext::currentContext() == context() );

    if ( index >= (int)_imp->pboIds.size() ) {
        GLuint handle;
        GL_GPU::GenBuffers(1, &handle);
        _imp->pboIds.push_back(handle);

        return handle;
    } else {
        return _imp->pboIds[index];
    }
}

RangeD
ViewerGL::getFrameRange() const
{
    int first, last;
    getViewerTab()->getTimelineBounds(&first, &last);
    RangeD ret;
    ret.min = first;
    ret.max = last;
    return ret;
}

ViewerNodePtr
ViewerGL::getViewerNode() const
{
    return getInternalNode();
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
RectD
ViewerGL::getImageRectangleDisplayed() const
{

    RectD visibleArea;

    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF topLeft =  _imp->zoomCtx.toZoomCoordinates(0, 0);
        QPointF bottomRight = _imp->zoomCtx.toZoomCoordinates(width() - 1, height() - 1);
        visibleArea.x1 =  topLeft.x();
        visibleArea.y2 =  topLeft.y();
        visibleArea.x2 = bottomRight.x();
        visibleArea.y1 = bottomRight.y();
    }

    return visibleArea;
} // ViewerGL::getImageRectangleDisplayed

int
ViewerGL::isExtensionSupported(const char *extension)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;
    where = (GLubyte *) std::strchr(extension, ' ');
    if ( where || (*extension == '\0') ) {
        return 0;
    }
    extensions = GL_GPU::GetString(GL_EXTENSIONS);
    start = extensions;
    for (;; ) {
        where = (GLubyte *) strstr( (const char *) start, extension );
        if (!where) {
            break;
        }
        terminator = where + std::strlen(extension);
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
ViewerGL::clearPartialUpdateTextures()
{
    QMutexLocker k(&_imp->displayDataMutex);
    _imp->partialUpdateTextures.clear();
}

bool
ViewerGL::isViewerUIVisible() const
{
    if (!_imp->viewerTab) {
        return false;
    }
    TabWidget* tabWidget = _imp->viewerTab->getParentPane();
    if (!tabWidget) {
        return isVisible();
    }

    if (tabWidget->currentWidget() != _imp->viewerTab) {
        return false;
    }

    return true;
}

void
ViewerGL::refreshMetadata(int inputNb, const NodeMetadata& metadata) 
{

    RectI inputFormat = metadata.getOutputFormat();

    double inputPar = metadata.getPixelAspectRatio(-1);

    {
        RectD inputCanonicalFormat;
        inputFormat.toCanonical_noClipping(0, inputPar, &inputCanonicalFormat);
        getViewerTab()->setInfoBarAndViewerResolution(inputFormat, inputCanonicalFormat, inputPar, inputNb);
    }

    _imp->displayTextures[inputNb].premult = metadata.getOutputPremult();
    ImagePlaneDesc upstreamLayer = ImagePlaneDesc::mapNCompsToColorPlane(metadata.getColorPlaneNComps(0));
    getViewerTab()->setImageFormat(inputNb, upstreamLayer, metadata.getBitDepth(0));
}

void
ViewerGL::clearLastRenderedImage()
{
    QMutexLocker k(&_imp->displayDataMutex);
    for (int i = 0; i < 2; ++i) {
        _imp->displayTextures[i].colorPickerImage.reset();
        _imp->displayTextures[i].colorPickerInputImage.reset();
    }
}

void
ViewerGL::getViewerProcessHashStored(ViewerCachedImagesMap* hashes) const
{
    QMutexLocker k(&_imp->uploadedTexturesViewerHashMutex);
    *hashes = _imp->uploadedTexturesViewerHash;
}


void
ViewerGL::removeViewerProcessHashAtTime(TimeValue time, ViewIdx view)
{
    QMutexLocker k(&_imp->uploadedTexturesViewerHashMutex);
    FrameViewPair p = {time, view};
    ViewerCachedImagesMap::iterator found = _imp->uploadedTexturesViewerHash.find(p);
    if (found != _imp->uploadedTexturesViewerHash.end()) {
        _imp->uploadedTexturesViewerHash.erase(found);
    }
}


void
ViewerGL::transferBufferFromRAMtoGPU(const TextureTransferArgs& args)
{
    // always running in the main thread
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(_imp->glContextWrapper);
    locker->attach();
    
    assert(QGLContext::currentContext() == context());

    glCheckError(GL_GPU);

    GLint currentBoundPBO = 0;
    GL_GPU::GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &currentBoundPBO);
    glCheckError(GL_GPU);

    // We use 2 PBOs to make use of asynchronous data uploading
    GLuint pboId = getPboID(_imp->updateViewerPboIndex);

    assert(args.textureIndex == 0 || args.textureIndex == 1);

    // Only RAM RGBA images at this point can be provided
    assert(!args.image || (args.image->getStorageMode() == eStorageModeRAM && args.image->getLayer().getNumComponents() == 4));

    // The bitdepth of the texture
    ImageBitDepthEnum bitdepth = eImageBitDepthFloat;
    if (args.image) {
        bitdepth = args.image->getBitDepth();
    }

    // Other formats are not supported yet
    assert(bitdepth == eImageBitDepthByte || bitdepth == eImageBitDepthFloat);

    Image::CPUData imageData;
    if (args.image) {
        args.image->getCPUData(&imageData);
    }


    // Insert the hash in the frame/hash map so we can update the timeline's cache bar
    if (args.type == TextureTransferArgs::eTextureTransferTypeReplace && args.textureIndex == 0 && args.viewerProcessNodeKey) {
        QMutexLocker k(&_imp->uploadedTexturesViewerHashMutex);
        FrameViewPair p = {args.time, args.view};
        _imp->uploadedTexturesViewerHash[p] = args.viewerProcessNodeKey;
    }


    GLTexturePtr tex;
    {
        QMutexLocker displayDataLocker(&_imp->displayDataMutex);
        if (args.type == TextureTransferArgs::eTextureTransferTypeOverlay) {

            if (!args.image) {
                // Nothing to do
                return;
            }

            // For small partial updates overlays, we make new textures
            int format, internalFormat, glType;

            if (bitdepth == eImageBitDepthFloat) {
                Texture::getRecommendedTexParametersForRGBAFloatTexture(&format, &internalFormat, &glType);
            } else {
                Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
            }
            tex.reset( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, bitdepth, format, internalFormat, glType, true) );

            TextureInfo info;
            info.texture = tex;
            info.mipMapLevel = args.image ? args.image->getMipMapLevel() : 0;
            info.premult = _imp->displayTextures[0].premult;
            info.pixelAspectRatio = _imp->displayTextures[0].pixelAspectRatio;
            info.time = args.time;
            info.isPartialImage = true;
            info.isVisible = true;
            _imp->partialUpdateTextures.push_back(info);

            // Update time otherwise overlays won't refresh since we are not updating the displayTextures
            _imp->displayTextures[0].time = args.time;
            _imp->displayTextures[1].time = args.time;
        } else {

            if (args.type == TextureTransferArgs::eTextureTransferTypeReplace) {
                _imp->displayTextures[args.textureIndex].colorPickerImage = args.colorPickerImage;
                _imp->displayTextures[args.textureIndex].colorPickerInputImage = args.colorPickerInputImage;
            }
            // re-use the existing texture if possible
            if (!args.image) {
                _imp->displayTextures[args.textureIndex].isVisible = false;
            } else {

                int format, internalFormat, glType;

                if (bitdepth == eImageBitDepthFloat) {
                    Texture::getRecommendedTexParametersForRGBAFloatTexture(&format, &internalFormat, &glType);
                } else {
                    Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
                }

                if (_imp->displayTextures[args.textureIndex].texture->getBitDepth() != bitdepth) {
                    _imp->displayTextures[args.textureIndex].texture.reset( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, bitdepth, format, internalFormat, glType, true) );
                }

                tex = _imp->displayTextures[args.textureIndex].texture;


                if (args.type == TextureTransferArgs::eTextureTransferTypeReplace || tex->getBounds().isNull()) {
                    tex->ensureTextureHasSize(imageData.bounds, 0);
                } else {
                    assert(args.type == TextureTransferArgs::eTextureTransferTypeModify);
                    // If we just want to update a portion of the texture, check if we are inside the bounds of the texture, otherwise create a new one.
                    if (!tex->getBounds().contains(imageData.bounds)) {
                        RectI unionedBounds = tex->getBounds();
                        unionedBounds.merge(imageData.bounds);


                        // Make a temporary texture, fill it with black and copy the origin texture into it before uploading the image
                        GLTexturePtr tmpTex = boost::make_shared<Texture>(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, bitdepth, format, internalFormat, glType, true);
                        tmpTex->ensureTextureHasSize(unionedBounds, 0);

                        saveOpenGLContext();

                        ImagePrivate::fillGL(unionedBounds, 0., 0., 0., 0., tmpTex, _imp->glContextWrapper);
                        ImagePrivate::copyGLTexture(tex, tmpTex, tex->getBounds(), _imp->glContextWrapper);

                        restoreOpenGLContext();
                        // Unbind the frame buffer used in fillGL and copyGLTexture
                        GL_GPU::BindFramebuffer(GL_FRAMEBUFFER, 0);

                        _imp->displayTextures[args.textureIndex].texture = tmpTex;
                        tex = tmpTex;

                    }
                }


                _imp->displayTextures[args.textureIndex].isVisible = true;
                _imp->displayTextures[args.textureIndex].mipMapLevel = args.image->getMipMapLevel();
                _imp->displayTextures[args.textureIndex].time = args.time;
            }

            displayDataLocker.unlock();
            if (args.colorPickerImage) {
                getViewerTab()->setImageFormat(args.textureIndex, args.colorPickerImage->getLayer(), args.colorPickerImage->getBitDepth());
            }
            setRegionOfDefinition(args.rod, _imp->displayTextures[args.textureIndex].pixelAspectRatio, args.textureIndex);
            if (args.type == TextureTransferArgs::eTextureTransferTypeReplace) {

                Q_EMIT imageChanged(args.textureIndex);
            }
        }
    } // displayDataLocker

    if (args.recenterViewer) {
        QMutexLocker k(&_imp->zoomCtxMutex);
        double curCenterX = ( _imp->zoomCtx.left() + _imp->zoomCtx.right() ) / 2.;
        double curCenterY = ( _imp->zoomCtx.bottom() + _imp->zoomCtx.top() ) / 2.;
        _imp->zoomCtx.translate(args.viewportCenter.x - curCenterX, args.viewportCenter.y - curCenterY);
    }

    if (!args.image) {
        return;
    }

    // bind PBO to update texture source
    GL_GPU::BindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboId );

    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // until GPU to finish its job. To avoid waiting (idle), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    int dataSizeOf = getSizeOfForBitDepth(imageData.bitDepth);
    std::size_t bytesCount = imageData.bounds.area() * imageData.nComps * dataSizeOf;
    assert(bytesCount > 0);
    GL_GPU::BufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);

    // map the buffer object into client's memory
    assert(QGLContext::currentContext() == context());
    GLvoid *ret = GL_GPU::MapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError(GL_GPU);
    assert(ret);
    if (ret) {
        // update data directly on the mapped buffer
        unsigned char* srcpixels = Image::pixelAtStatic(imageData.bounds.x1, imageData.bounds.y1, imageData.bounds, imageData.nComps, dataSizeOf, (unsigned char*)imageData.ptrs[0]);
        assert(srcpixels);
        if (srcpixels) {
            std::memcpy(ret, srcpixels, bytesCount);
            GLboolean result = GL_GPU::UnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
            assert(result == GL_TRUE);
            Q_UNUSED(result);
        }
    }
    glCheckError(GL_GPU);

    // copy pixels from PBO to texture object
    // using glBindTexture followed by glTexSubImage2D.
    // Use offset instead of pointer (last parameter is 0).
    tex->fillOrAllocateTexture(imageData.bounds, 0, 0);

    // restore previously bound PBO
    GL_GPU::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);
    //glBindTexture(GL_TEXTURE_2D, 0); // why should we bind texture 0?
    glCheckError(GL_GPU);

    _imp->updateViewerPboIndex = (_imp->updateViewerPboIndex + 1) % 2;

} // ViewerGL::transferBufferFromRAMtoGPU


void
ViewerGL::disconnectInputTexture(int textureIndex, bool clearRoD)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QMutexLocker k(&_imp->displayDataMutex);
    assert(textureIndex == 0 || textureIndex == 1);
    if (_imp->displayTextures[textureIndex].isVisible) {
        _imp->displayTextures[textureIndex].isVisible = false;
        if (clearRoD) {
            RectI r(0, 0, 0, 0);
            _imp->infoViewer[textureIndex]->setDataWindow(r);
        }
    }
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

    _imp->viewerTab->onMousePressCalledInViewer();

    _imp->hasMovedSincePress = false;
    _imp->pressureOnRelease = 1.;
    if ( (e->buttons() == Qt::LeftButton) && !(e->modifiers() &  Qt::MetaModifier) ) {
        _imp->pointerTypeOnPress = ePenTypeLMB;
    } else if ( (e->buttons() == Qt::RightButton) || (e->buttons() == Qt::LeftButton  && (e->modifiers() &  Qt::MetaModifier)) ) {
        _imp->pointerTypeOnPress = ePenTypeRMB;
    } else if ( (e->buttons() == Qt::MiddleButton)  || (e->buttons() == Qt::LeftButton  && (e->modifiers() &  Qt::AltModifier)) ) {
        _imp->pointerTypeOnPress = ePenTypeMMB;
    }


    ///Set focus on user click
    setFocus();

    Qt::MouseButton button = e->button();

    if ( buttonDownIsLeft(e) ) {
        NodeGuiIPtr gui_i = _imp->viewerTab->getInternalNode()->getNode()->getNodeGui();
        assert(gui_i);
        NodeGuiPtr gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
        _imp->viewerTab->getGui()->selectNode(gui);
    }

    _imp->oldClick = e->pos();
    _imp->lastMousePosition = e->pos();
    QPointF zoomPos;
    //double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        //zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        //zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }

    bool overlaysCaught = false;
    bool mustRedraw = false;
    bool hasPickers = _imp->viewerTab->getGui()->hasPickers();

    if ( !overlaysCaught &&
         ( buttonDownIsMiddle(e) ||
           ( ( (e)->buttons() == Qt::RightButton ) && ( buttonMetaAlt(e) == Qt::AltModifier) ) ) &&
         !modifierHasControl(e) ) {
        // middle (or Alt + left) or Alt + right = pan
        _imp->ms = eMouseStateDraggingImage;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
         ( ( (e->buttons() & Qt::MiddleButton) &&
             ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
           ( (e->buttons() & Qt::LeftButton) &&
             ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) ) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->ms = eMouseStateZoomingImage;
        overlaysCaught = true;
    }

    ViewerNodePtr viewerNode = getInternalNode();
    bool overlayEnabled = viewerNode->isOverlayEnabled();

    // process plugin overlays
    if (!overlaysCaught &&
        (_imp->ms == eMouseStateUndefined) &&
        overlayEnabled) {
        unsigned int mipMapLevel = getCurrentRenderScale();
        double scale = 1. / (1 << mipMapLevel);
        overlaysCaught = _imp->viewerTab->notifyOverlaysPenDown( RenderScale(scale), _imp->pointerTypeOnPress, QMouseEventLocalPos(e), zoomPos, _imp->pressureOnPress, TimeValue(currentTimeForEvent(e)) );
        if (overlaysCaught) {
            mustRedraw = true;
        }
    }

    if ( !overlaysCaught &&
        button == Qt::LeftButton && modCASIsControl(e) &&
        displayingImage() ) {
        // picker with single-point selection
        _imp->pickerState = ePickerStatePoint;
        if ( pickColor( e->x(), e->y(), false ) ) {
            _imp->ms = eMouseStatePickingColor;
            mustRedraw = true;
            overlaysCaught = true;
        }
    }

    if ( !overlaysCaught &&
        button == Qt::LeftButton && modCASIsControlAlt(e) &&
        displayingImage() ) {
        // picker with single-point selection
        _imp->pickerState = ePickerStatePoint;
        if ( pickColor( e->x(), e->y(), true ) ) {
            _imp->ms = eMouseStatePickingInputColor;
            mustRedraw = true;
            overlaysCaught = true;
        }
    }

    if ( !overlaysCaught &&
         button == Qt::LeftButton && modCASIsControlShift(e) &&
         displayingImage() ) {
        // start picker with rectangle selection (picked color is the average over the rectangle)
        _imp->pickerState = ePickerStateRectangle;
        _imp->pickerRect.setTopLeft(zoomPos);
        _imp->pickerRect.setBottomRight(zoomPos);
        _imp->ms = eMouseStateBuildingPickerRectangle;
        mustRedraw = true;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
         (_imp->pickerState != ePickerStateInactive) &&
         buttonDownIsLeft(e) &&
         displayingImage() ) {
        // disable picker if picker is set when clicking
        _imp->pickerState = ePickerStateInactive;
        mustRedraw = true;
        overlaysCaught = true;
    }

    if ( hasPickers && (_imp->pickerState == ePickerStateInactive) ) {
        _imp->viewerTab->getGui()->clearColorPickers();
        unsetCursor();
    }

    if ( !overlaysCaught &&
         buttonDownIsLeft(e) ) {
        ///build selection rectangle
        _imp->selectionRectangle.setTopLeft(zoomPos);
        _imp->selectionRectangle.setBottomRight(zoomPos);
        _imp->lastDragStartPos = zoomPos;
        _imp->ms = eMouseStateSelecting;
        if ( !modCASIsControl(e) ) {
            _imp->viewerTab->onViewerSelectionCleared();
            mustRedraw = true;
        }
        overlaysCaught = true;
    }
    Q_UNUSED(overlaysCaught);

    if (mustRedraw) {
        update();
    }
} // mousePressEvent

void
ViewerGL::mouseReleaseEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !_imp->viewerTab->getGui() ) {
        return;
    }

    _imp->pressureOnPress = 1;
    _imp->pointerTypeOnPress = ePenTypeLMB;

    bool mustRedraw = false;
    if (_imp->ms == eMouseStateBuildingPickerRectangle) {
        updateRectangleColorPicker();
    }

    if (_imp->ms == eMouseStateSelecting) {
        mustRedraw = true;

        if (_imp->hasMovedSincePress) {
            _imp->viewerTab->updateSelectionFromViewerSelectionRectangle(true);
        }
    }

    _imp->hasMovedSincePress = false;


    _imp->ms = eMouseStateUndefined;
    QPointF zoomPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
    }
    unsigned int mipMapLevel = getCurrentRenderScale();
    double scale = 1. / (1 << mipMapLevel);
    if ( _imp->viewerTab->notifyOverlaysPenUp(RenderScale(scale), QMouseEventLocalPos(e), zoomPos, _imp->pressureOnRelease, TimeValue(currentTimeForEvent(e))) ) {
        mustRedraw = true;
    }
    if (mustRedraw) {
        update();
    }

    if (_imp->renderOnPenUp) {
        _imp->renderOnPenUp = false;
        getInternalNode()->getNode()->getRenderEngine()->renderCurrentFrame();
    }
} // ViewerGL::mouseReleaseEvent

void
ViewerGL::mouseMoveEvent(QMouseEvent* e)
{
    if ( !penMotionInternal(e->x(), e->y(), /*pressure=*/ 1., TimeValue(currentTimeForEvent(e)), e) ) {
        //e->ignore(); //< calling e->ignore() is the same as calling the base implementation of QWidget::mouseMoveEvent(e)
        TabWidget* tab = _imp->viewerTab ? _imp->viewerTab->getParentPane() : 0;
        if (tab) {
            // If the Viewer is in a tab, send the tab widget the event directly
            qApp->sendEvent(tab, e);
        } else {
            QGLWidget::mouseMoveEvent(e);
        }
    }
} // mouseMoveEvent

void
ViewerGL::tabletEvent(QTabletEvent* e)
{
    qreal pressure = e->pressure();
    if (pressure <= 0.) {
        // Some tablets seem to return pressure between -0.5 and 0.5
        // This is probably a Qt bug, see https://github.com/MrKepzie/Natron/issues/1697
        pressure += 1.;
    }
    switch ( e->type() ) {
    case QEvent::TabletPress: {
        switch ( e->pointerType() ) {
        case QTabletEvent::Cursor:
            _imp->pointerTypeOnPress  = ePenTypeCursor;
            break;
        case QTabletEvent::Eraser:
            _imp->pointerTypeOnPress  = ePenTypeEraser;
            break;
        case QTabletEvent::Pen:
        default:
            _imp->pointerTypeOnPress  = ePenTypePen;
            break;
        }
        _imp->pressureOnPress = pressure;
        QGLWidget::tabletEvent(e);
        break;
    }
    case QEvent::TabletRelease: {
        _imp->pressureOnRelease = pressure;
        QGLWidget::tabletEvent(e);
        break;
    }
    case QEvent::TabletMove: {
        if ( !penMotionInternal(e->x(), e->y(), pressure, TimeValue(currentTimeForEvent(e)), e) ) {
            QGLWidget::tabletEvent(e);
        } else {
            e->accept();
        }
        break;
    }
    default:
        break;
    }
}

bool
ViewerGL::penMotionInternal(int x,
                            int y,
                            double pressure,
                            TimeValue timestamp,
                            QInputEvent* e)
{
    Q_UNUSED(e);
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///The app is closing don't do anything
    Gui* gui = _imp->viewerTab->getGui();
    if ( !gui || !getInternalNode() ) {
        return false;
    }

    _imp->hasMovedSincePress = true;

    QPointF zoomPos;

    // if the picker was deselected, this fixes the picer State
    // (see issue #133 https://github.com/MrKepzie/Natron/issues/133 )
    // Edited: commented-out we need the picker state to be active even if there are no color pickers active because parametric
    // parameters may use it
    //if ( !gui->hasPickers() ) {
    //    _imp->pickerState = ePickerStateInactive;
    //}

    //double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates(x, y);
    }

    ViewerNodePtr viewerNode = getInternalNode();

    updateInfoWidgetColorPicker( zoomPos, QPoint(x, y) );
    if ( viewerNode->isViewersSynchroEnabled() ) {
        const std::list<ViewerTab*>& allViewers = gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = allViewers.begin(); it != allViewers.end(); ++it) {
            if ( (*it)->getViewer() != this ) {
                (*it)->getViewer()->updateInfoWidgetColorPicker( zoomPos, QPoint(x, y) );
            }
        }
    }

    //update the cursor if it is hovering an overlay and we're not dragging the image
    RectD userRoI = viewerNode->getUserRoI();

    bool mustRedraw = false;


    QPoint newClick(x, y);
    QPoint oldClick = _imp->oldClick;
    QPointF newClick_opengl, oldClick_opengl, oldPosition_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        newClick_opengl = _imp->zoomCtx.toZoomCoordinates( newClick.x(), newClick.y() );
        oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( oldClick.x(), oldClick.y() );
        oldPosition_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->lastMousePosition.x(), _imp->lastMousePosition.y() );
    }
    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );
    double dxSinceLastMove = ( oldPosition_opengl.x() - newClick_opengl.x() );
    double dySinceLastMove = ( oldPosition_opengl.y() - newClick_opengl.y() );
    bool overlaysCaughtByPlugin = false;
    switch (_imp->ms) {
    case eMouseStateDraggingImage: {

        translateViewport(dx, dy);
        _imp->oldClick = newClick;

        //  else {
        mustRedraw = true;
        // }
        // no need to update the color picker or mouse posn: they should be unchanged
        break;
    }
    case eMouseStateZoomingImage: {
        const double zoomFactor_min = 0.01;
        const double zoomFactor_max = 1024.;
        double zoomFactor;
        int delta = 2 * ( ( x - _imp->lastMousePosition.x() ) - ( y - _imp->lastMousePosition.y() ) );
        double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, delta);
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
            if (zoomFactor <= zoomFactor_min) {
                zoomFactor = zoomFactor_min;
                scaleFactor = zoomFactor / _imp->zoomCtx.factor();
            } else if (zoomFactor > zoomFactor_max) {
                zoomFactor = zoomFactor_max;
                scaleFactor = zoomFactor / _imp->zoomCtx.factor();
            }
            _imp->zoomCtx.zoom(oldClick_opengl.x(), oldClick_opengl.y(), scaleFactor);
            _imp->zoomOrPannedSinceLastFit = true;
        }
        int zoomValue = (int)(100 * zoomFactor);
        if (zoomValue == 0) {
            zoomValue = 1;     // sometimes, floor(100*0.01) makes 0
        }
        assert(zoomValue > 0);
        Q_EMIT zoomChanged(zoomValue);

        ViewerNodePtr viewerNode = getInternalNode();
        if ( viewerNode->isViewersSynchroEnabled() ) {
            _imp->viewerTab->synchronizeOtherViewersProjection();
        }

        //_imp->oldClick = newClick; // don't update oldClick! this is the zoom center
        getInternalNode()->getNode()->getRenderEngine()->renderCurrentFrame();

        //  else {
        mustRedraw = true;
        // }
        // no need to update the color picker or mouse posn: they should be unchanged
        break;
    }
    case eMouseStatePickingColor: {
        pickColor( newClick.x(), newClick.y(), false );
        mustRedraw = true;
        break;
    }
    case eMouseStatePickingInputColor: {
        pickColor( newClick.x(), newClick.y(), true );
        mustRedraw = true;
        break;
    }
    case eMouseStateBuildingPickerRectangle: {
        QPointF btmRight = _imp->pickerRect.bottomRight();
        btmRight.rx() -= dxSinceLastMove;
        btmRight.ry() -= dySinceLastMove;
        _imp->pickerRect.setBottomRight(btmRight);
        mustRedraw = true;
        break;
    }
    case eMouseStateSelecting: {
        _imp->refreshSelectionRectangle(zoomPos);
        mustRedraw = true;
        _imp->viewerTab->updateSelectionFromViewerSelectionRectangle(false);
    }; break;
    default: {
        QPointF localPos(x, y);
        unsigned int mipMapLevel = getCurrentRenderScale();
        double scale = 1. / (1 << mipMapLevel);
        bool overlayEnabled = viewerNode->isOverlayEnabled();
        if ( overlayEnabled &&
             _imp->viewerTab->notifyOverlaysPenMotion(RenderScale(scale), localPos, zoomPos, pressure, timestamp) ) {
            mustRedraw = true;
            overlaysCaughtByPlugin = true;
        }
        break;
    }
    } // switch

    if ( _imp->viewerTab->getGui()->hasPickers() ) {
        setCursor( appPTR->getColorPickerCursor() );
    } else if (!overlaysCaughtByPlugin) {
        unsetCursor();
    }

    if (mustRedraw) {
        update();
    }
    _imp->lastMousePosition = newClick;
    
    return mustRedraw;
} // ViewerGL::penMotionInternal

void
ViewerGL::mouseDoubleClickEvent(QMouseEvent* e)
{
    unsigned int mipMapLevel;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        mipMapLevel = _imp->displayTextures[0].mipMapLevel;
    }
    QPointF pos_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        pos_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
    }
    double scale = 1. / (1 << mipMapLevel);

    if ( _imp->viewerTab->notifyOverlaysPenDoubleClick(RenderScale(scale), QMouseEventLocalPos(e), pos_opengl) ) {
        update();
    }
    QGLWidget::mouseDoubleClickEvent(e);
}

QPointF
ViewerGL::toZoomCoordinates(const QPointF& position) const
{
    QMutexLocker l(&_imp->zoomCtxMutex);

    return _imp->zoomCtx.toZoomCoordinates( position.x(), position.y() );
}

QPointF
ViewerGL::toWidgetCoordinates(const QPointF& position) const
{
    QMutexLocker l(&_imp->zoomCtxMutex);

    return _imp->zoomCtx.toWidgetCoordinates( position.x(), position.y() );
}

void
ViewerGL::getTopLeftAndBottomRightInZoomCoords(QPointF* topLeft,
                                               QPointF* bottomRight) const
{
    QMutexLocker l(&_imp->zoomCtxMutex);

    *topLeft = _imp->zoomCtx.toZoomCoordinates(0, 0);
    *bottomRight = _imp->zoomCtx.toZoomCoordinates(_imp->zoomCtx.screenWidth() - 1, _imp->zoomCtx.screenHeight() - 1);
}

// used to update the information bar at the bottom of the viewer (not for the ctrl-click color picker)
void
ViewerGL::updateColorPicker(int textureIndex,
                            int x,
                            int y)
{
    if ( (_imp->pickerState != ePickerStateInactive) || !_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->isGUIFrozen() ) {
        return;
    }

    const std::list<Histogram*>& histograms = _imp->viewerTab->getGui()->getHistograms();

    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( !displayingImage() ) {
        for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
            if ( (*it)->getViewerTextureInputDisplayed() == textureIndex ) {
                (*it)->hideViewerCursor();
            }
        }
        if ( _imp->infoViewer[textureIndex]->colorVisible() ) {
            _imp->infoViewer[textureIndex]->hideColorInfo();
        }
        if (textureIndex == 0) {
            setParametricParamsPickerColor(ColorRgba<double>(), false, false);
        }
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

    ViewerNodePtr viewerNode = getInternalNode();
    QPointF imgPosCanonical;
    bool imgPosCanonicalSet = false;
    if (!xInitialized || !yInitialized) {
        if ( viewerNode->isViewersSynchroEnabled() ) {
            NodePtr masterViewerNode = getViewerTab()->getGui()->getApp()->getMasterSyncViewer();
            if (masterViewerNode) {
                ViewerNodePtr viewerNode = toViewerNode(masterViewerNode->getEffectInstance());
                ViewerGL* viewerUIContext = dynamic_cast<ViewerGL*>(viewerNode->getUiContext());
                assert(viewerUIContext);
                if (viewerUIContext) {
                    pos = viewerUIContext->mapFromGlobal( QCursor::pos() );
                    xInitialized = yInitialized = true;
                    imgPosCanonical = viewerUIContext->toZoomCoordinates(pos);
                    imgPosCanonicalSet = true;
                }
            }
        }
    }
    if (!xInitialized || !yInitialized) {
        pos = mapFromGlobal( QCursor::pos() );
    }
    if (!imgPosCanonicalSet) {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPosCanonical = _imp->zoomCtx.toZoomCoordinates( pos.x(), pos.y() );
    }

    float r, g, b, a;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool picked = false;
    bool pickInput = (qApp->keyboardModifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier)) == (Qt::ControlModifier | Qt::AltModifier);
    RectD rod = getRoD(textureIndex);
    RectD formatCanonical = getCanonicalFormat(textureIndex);
    unsigned int mmLevel;
    if ( ( imgPosCanonical.x() >= rod.left() ) &&
         ( imgPosCanonical.x() < rod.right() ) &&
         ( imgPosCanonical.y() >= rod.bottom() ) &&
         ( imgPosCanonical.y() < rod.top() ) ) {
        if ( (pos.x() >= 0) && ( pos.x() < width() ) &&
             (pos.y() >= 0) && ( pos.y() < height() ) ) {
            ///if the clip to project format is enabled, make sure it is in the project format too
            bool clipping = viewerNode->isClipToFormatEnabled();
            if ( !clipping ||
                 ( ( imgPosCanonical.x() >= formatCanonical.left() ) &&
                   ( imgPosCanonical.x() < formatCanonical.right() ) &&
                   ( imgPosCanonical.y() >= formatCanonical.bottom() ) &&
                   ( imgPosCanonical.y() < formatCanonical.top() ) ) ) {
                //imgPos must be in canonical coordinates
                picked = getColorAt(imgPosCanonical.x(), imgPosCanonical.y(), linear, textureIndex, pickInput, &r, &g, &b, &a, &mmLevel);
            }
        }
    }
    if (!picked) {
        _imp->infoViewer[textureIndex]->setColorValid(false);
        if (textureIndex == 0) {
            setParametricParamsPickerColor(ColorRgba<double>(), false, false);
        }
    } else {
        _imp->infoViewer[textureIndex]->setColorApproximated(mmLevel > 0);
        _imp->infoViewer[textureIndex]->setColorValid(true);
        if ( !_imp->infoViewer[textureIndex]->colorVisible() ) {
            _imp->infoViewer[textureIndex]->showColorInfo();
        }
        _imp->infoViewer[textureIndex]->setColor(r, g, b, a);

        if (textureIndex == 0) {
            ColorRgba<double> interactColor(r,g,b,a);
            setParametricParamsPickerColor(interactColor, true, true);
        }

        std::vector<double> colorVec(4);
        colorVec[0] = r;
        colorVec[1] = g;
        colorVec[2] = b;
        colorVec[3] = a;
        for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
            if ( (*it)->getViewerTextureInputDisplayed() == textureIndex ) {
                (*it)->setViewerCursor(colorVec);
            }
        }
    }
} // updateColorPicker

void
ViewerGL::setParametricParamsPickerColor(const ColorRgba<double>& color, bool setColor, bool hasColor)
{
    if (!_imp->viewerTab->getGui()) {
        return;
    }
    std::list<DockablePanelI*> openedPanels = _imp->viewerTab->getGui()->getApp()->getOpenedSettingsPanels();
    for (std::list<DockablePanelI*>::const_iterator it = openedPanels.begin(); it != openedPanels.end(); ++it) {
        NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(*it);
        if (!nodePanel) {
            continue;
        }
        NodeGuiPtr node = nodePanel->getNodeGui();
        if (!node) {
            continue;
        }
        node->getNode()->getEffectInstance()->setInteractColourPicker_public(color, setColor, hasColor);
    }
}


int
ViewerGL::getMipMapLevelFromZoomFactor() const
{
    double zoomFactor = getZoomFactor();
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );
    return std::log(closestPowerOf2) / M_LN2;
}

bool
ViewerGL::checkIfViewPortRoIValidOrRenderForInput(int texIndex)
{

    ViewerNodePtr viewerNode = getInternalNode();

    unsigned int mipMapLevel;
    {
        int downscale_i = viewerNode->getDownscaleMipMapLevelKnobIndex();
        if (downscale_i == 0) {
            mipMapLevel = getMipMapLevelFromZoomFactor();
        } else {
            mipMapLevel = downscale_i;
        }
    }


    RectD roiCanonical = viewerNode->getViewerProcessNode(texIndex)->getViewerRoI();

    QMutexLocker k(&_imp->displayDataMutex);
    if (_imp->displayTextures[texIndex].mipMapLevel != mipMapLevel) {
        return false;
    }

    RectI roiPixel;
    roiCanonical.toPixelEnclosing(_imp->displayTextures[texIndex].mipMapLevel, _imp->displayTextures[texIndex].pixelAspectRatio, &roiPixel);


    int tx,ty;
    CacheBase::getTileSizePx(_imp->displayTextures[texIndex].texture->getBitDepth(), &tx, &ty);


    const RectI& texBounds = _imp->displayTextures[texIndex].texture->getBounds();
    if (!texBounds.contains(roiPixel)) {
        return false;
    }

    return true;
}

void
ViewerGL::checkIfViewPortRoIValidOrRender()
{
    for (int i = 0; i < 2; ++i) {
        if (!_imp->displayTextures[i].isVisible) {
            continue;
        }
        if ( !checkIfViewPortRoIValidOrRenderForInput(i) ) {
            if ( !getViewerTab()->getGui()->getApp()->getProject()->isLoadingProject() ) {
                ViewerNodePtr viewer = getInternalNode();
                assert(viewer);
                if (viewer) {
                    viewer->getNode()->getRenderEngine()->abortRenderingAutoRestart();
                    viewer->getNode()->getRenderEngine()->renderCurrentFrame();
                }
            }
            break;
        }
    }
}

void
ViewerGL::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (!_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->getApp()->getActiveRotoDrawingStroke()) {
        return QGLWidget::wheelEvent(e);
    }

    // delta=120 is a standard wheel mouse click, but mice may be more accurate
    // - Apple Trackpad has an accuracy of delta=2
    // - Apple Magic Mouse has an accuracy of delta=2
    // - Apple Mighty Mouse has an accuracy of delta=28
    // - Logitech Wheel Mouse has an accuracy of delta=120
    // The Qt Unit is 8 deltas per degree.
    // delta/2 gives a reasonable value in pixels (for panning, etc).

    // frame seeking works both with horizontal and vertical wheel + control modifier
    if ( modCASIsControl(e) ) {
        const int delta_max = 28;
        // threshold delta to the range -delta_max..delta_max
        int delta = std::max( -delta_max, std::min(e->delta(), delta_max) );
        _imp->wheelDeltaSeekFrame += delta;
        if (_imp->wheelDeltaSeekFrame <= -delta_max) {
            _imp->wheelDeltaSeekFrame += delta_max;
            _imp->viewerTab->nextFrame();
        } else if (_imp->wheelDeltaSeekFrame >= delta_max) {
            _imp->wheelDeltaSeekFrame -= delta_max;
            _imp->viewerTab->previousFrame();
        }

        return;
    }

    if (e->orientation() != Qt::Vertical) {
        // we only handle vertical motion for zooming
        return QGLWidget::wheelEvent(e);
    }


    Gui* gui = _imp->viewerTab->getGui();
    if (!gui) {
        return QGLWidget::wheelEvent(e);
    }

    NodeGuiIPtr nodeGui_i = _imp->viewerTab->getInternalNode()->getNode()->getNodeGui();
    NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>(nodeGui_i);
    gui->selectNode(nodeGui);

    const double zoomFactor_min = 0.01;
    const double zoomFactor_max = 1024.;
    double zoomFactor;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomFactor = _imp->zoomCtx.factor();

        zoomFactor *= scaleFactor;

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
    Q_EMIT zoomChanged(zoomValue);

    ViewerNodePtr viewerNode = getInternalNode();
    if ( viewerNode->isViewersSynchroEnabled() ) {
        _imp->viewerTab->synchronizeOtherViewersProjection();
    }

    checkIfViewPortRoIValidOrRender();

    update();
} // ViewerGL::wheelEvent

void
ViewerGL::translateViewport(double dx, double dy)
{
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        _imp->zoomCtx.translate(dx, dy);
        _imp->zoomOrPannedSinceLastFit = true;
    }

    ViewerNodePtr viewerNode = getInternalNode();
    if ( viewerNode->isViewersSynchroEnabled() ) {
        _imp->viewerTab->synchronizeOtherViewersProjection();
    }

    checkIfViewPortRoIValidOrRender();
}

void
ViewerGL::zoomViewport(double newZoomFactor)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    assert(newZoomFactor > 0);
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
        _imp->zoomOrPannedSinceLastFit = true;
    }
    _imp->viewerTab->updateZoomComboBox(std::floor(newZoomFactor * 100 + 0.5));
    checkIfViewPortRoIValidOrRender();
}


void
ViewerGL::fitImageToFormat()
{
    if (!_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->getApp()->getActiveRotoDrawingStroke()) {
        return;
    }

    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    // size in Canonical = Zoom coordinates !
    double w, h;
    const RectD& tex0Format = _imp->displayTextures[0].format;
    assert(!tex0Format.isNull());
    w = tex0Format.width();
    h = tex0Format.height();

    assert(h > 0. && w > 0.);

    double old_zoomFactor;
    double zoomFactor;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        old_zoomFactor = _imp->zoomCtx.factor();

        // set the PAR first
        //_imp->zoomCtx.setZoom(0., 0., 1., 1.);
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
        Q_EMIT zoomChanged(zoomFactorInt);
    }

    ViewerNodePtr viewerNode = getInternalNode();
    if ( viewerNode->isViewersSynchroEnabled() ) {
        _imp->viewerTab->synchronizeOtherViewersProjection();
    }

    checkIfViewPortRoIValidOrRender();

    update();
} // ViewerGL::fitImageToFormat


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
        Format f;
        getViewerTab()->getGui()->getApp()->getProject()->getProjectDefaultFormat(&f);
        RectD canonicalFormat = f.toCanonicalFormat();
        for (int i = 0; i < 2; ++i) {
            setRegionOfDefinition(canonicalFormat, f.getPixelAspectRatio(), i);
        }
    }
    //resetWipeControls();
    clearViewer();
}

/* The dataWindow of the currentFrame(BBOX) in canonical coordinates */
RectD
ViewerGL::getRoD(int textureIndex) const
{
    QMutexLocker k(&_imp->displayDataMutex);
    return _imp->displayTextures[textureIndex].rod;
}

/*The displayWindow of the currentFrame(Resolution)
   This is the same for both as we only use the display window as to indicate the project window.*/
RectD
ViewerGL::getCanonicalFormat(int texIndex) const
{
    QMutexLocker k(&_imp->displayDataMutex);
    return _imp->displayTextures[texIndex].format;
}

double
ViewerGL::getPAR(int texIndex) const
{
    QMutexLocker k(&_imp->displayDataMutex);
    return _imp->displayTextures[texIndex].pixelAspectRatio;
}

void
ViewerGL::setRegionOfDefinition(const RectD & rod,
                                double par,
                                int textureIndex)
{
    // always running in the main thread
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }

    RectI pixelRoD;
    rod.toPixelEnclosing(0, par, &pixelRoD);

    QMutexLocker k(&_imp->displayDataMutex);

    _imp->displayTextures[textureIndex].pixelAspectRatio = par;
    _imp->displayTextures[textureIndex].rod = rod;
    if ( _imp->infoViewer[textureIndex] && !_imp->viewerTab->getGui()->isGUIFrozen() ) {
        _imp->infoViewer[textureIndex]->setDataWindow(pixelRoD);
    }

    const RectI& r = pixelRoD;
    QString x1, y1, x2, y2;
    x1.setNum(r.x1);
    y1.setNum(r.y1);
    x2.setNum(r.x2);
    y2.setNum(r.y2);


    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex] = x1 + QLatin1Char(',') + y1;
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex] = x2 + QLatin1Char(',') + y2;
}

void
ViewerGL::setFormat(const std::string& formatName, const RectD& format, double par, int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }

    bool callFit = false;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        if (_imp->displayTextures[textureIndex].format != format || _imp->displayTextures[textureIndex].pixelAspectRatio != par) {
            _imp->displayTextures[textureIndex].format = format;
            _imp->displayTextures[textureIndex].pixelAspectRatio = par;
            if (!getZoomOrPannedSinceLastFit() && _imp->zoomCtx.screenWidth() != 0 && _imp->zoomCtx.screenHeight() != 0) {
                callFit = true;
            }
        }
        _imp->currentViewerInfo_resolutionOverlay[textureIndex] = QString::fromUtf8(formatName.c_str());

    }
    if (callFit) {
        fitImageToFormat();
    }

}

/*display black in the viewer*/
void
ViewerGL::clearViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        QMutexLocker k(&_imp->displayDataMutex);
        _imp->displayTextures[0].isVisible = false;
        _imp->displayTextures[1].isVisible = false;
    }
    update();
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
    double scale = 1. / ( 1 << getCurrentRenderScale() );
    if ( _imp->viewerTab->notifyOverlaysFocusGained( RenderScale(scale) ) ) {
        update();
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

    double scale = 1. / ( 1 << getCurrentRenderScale() );
    if ( _imp->viewerTab->notifyOverlaysFocusLost( RenderScale(scale) ) ) {
        update();
    }
    QGLWidget::focusOutEvent(e);
}

void
ViewerGL::enterEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->infoViewer[0]->showMouseInfo();
    _imp->infoViewer[1]->showMouseInfo();
    QGLWidget::enterEvent(e);
}

void
ViewerGL::leaveEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (_imp->pickerState == ePickerStateInactive) {
        _imp->infoViewer[0]->hideColorInfo();
        _imp->infoViewer[1]->hideColorInfo();
        setParametricParamsPickerColor(ColorRgba<double>(), false, false);
    }
    _imp->infoViewer[0]->hideMouseInfo();
    _imp->infoViewer[1]->hideMouseInfo();
    QGLWidget::leaveEvent(e);
}

QPixmap
ViewerGL::renderPixmap(int w, int h, bool useContext)
{
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(_imp->glContextWrapper);
    locker->attach();

    return QGLWidget::renderPixmap(w, h, useContext);
}

QImage
ViewerGL::grabFrameBuffer(bool withAlpha)
{
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(_imp->glContextWrapper);
    locker->attach();

    return QGLWidget::grabFrameBuffer(withAlpha);
}

void
ViewerGL::resizeEvent(QResizeEvent* e)
{ // public to hack the protected field
  // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(_imp->glContextWrapper);
    locker->attach();
    QGLWidget::resizeEvent(e);
}

bool
ViewerGL::renderText(double x,
                     double y,
                     const std::string &string,
                     double r,
                     double g,
                     double b,
                     double a,
                     int flags)
{
    QColor c;

    c.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    c.setAlphaF(Image::clamp(a, 0., 1.));
    renderText(x, y, QString::fromUtf8( string.c_str() ), c, font(), flags);

    return true;
}

void
ViewerGL::renderText(double x,
                     double y,
                     const QString &text,
                     const QColor &color,
                     const QFont &font,
                     int flags)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( text.isEmpty() ) {
        return;
    }
    double w = (double)width();
    double h = (double)height();
    double bottom;
    double left;
    double top;
    double right;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        bottom = _imp->zoomCtx.bottom();
        left = _imp->zoomCtx.left();
        top =  _imp->zoomCtx.top();
        right = _imp->zoomCtx.right();
    }
    if ( (w <= 0) || (h <= 0) || (right <= left) || (top <= bottom) ) {
        return;
    }
    double scalex = (right - left) / w;
    double scaley = (top - bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font, flags);
    glCheckError(GL_GPU);
}

void
ViewerGL::updatePersistentMessageToWidth(int w)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !_imp->viewerTab || !_imp->viewerTab->getGui() ) {
        return;
    }

    std::list<DockablePanelI*> openedPanels = _imp->viewerTab->getGui()->getApp()->getOpenedSettingsPanels();

    _imp->persistentMessages.clear();
    QStringList allMessages;
    int type = 0;
    ///Draw overlays in reverse order of appearance
    std::list<DockablePanelI*>::const_iterator next = openedPanels.begin();
    if ( next != openedPanels.end() ) {
        ++next;
    }
    int nbNonEmpty = 0;
    for (std::list<DockablePanelI*>::const_iterator it = openedPanels.begin(); it != openedPanels.end(); ++it) {
        const NodeSettingsPanel* isNodePanel = dynamic_cast<const NodeSettingsPanel*>(*it);
        if (!isNodePanel) {
            continue;
        }

        NodePtr node = isNodePanel->getNodeGui()->getNode();
        if (!node) {
            continue;
        }

        PersistentMessageMap messages;
        node->getPersistentMessage(&messages, true);

        for (PersistentMessageMap::const_iterator it2 = messages.begin(); it2 != messages.end(); ++it2) {
            if (it2->second.message.empty()) {
                continue;
            }

            allMessages.append(QString::fromUtf8(it2->second.message.c_str()));
            ++nbNonEmpty;
            type = (nbNonEmpty == 1 && it2->second.type == eMessageTypeWarning) ? eMessageTypeWarning : eMessageTypeError;

        }

        if ( next != openedPanels.end() ) {
            ++next;
        }


    }
    _imp->persistentMessageType = type;

    QFontMetrics fm(_imp->textFont);

    for (int i = 0; i < allMessages.size(); ++i) {
        QStringList wordWrapped = wordWrap(fm, allMessages[i], w - PERSISTENT_MESSAGE_LEFT_OFFSET_PIXELS);
        for (int j = 0; j < wordWrapped.size(); ++j) {
            _imp->persistentMessages.push_back(wordWrapped[j]);
        }
    }

    _imp->displayPersistentMessage = !_imp->persistentMessages.isEmpty();
    update();
} // ViewerGL::updatePersistentMessageToWidth

void
ViewerGL::updatePersistentMessage()
{
    updatePersistentMessageToWidth(width() - 20);
}

void
ViewerGL::getProjection(double *zoomLeft,
                        double *zoomBottom,
                        double *zoomFactor,
                        double *zoomAspectRatio) const
{
    // MT-SAFE
    QMutexLocker l(&_imp->zoomCtxMutex);

    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomAspectRatio = _imp->zoomCtx.aspectRatio();
}

void
ViewerGL::setProjection(double zoomLeft,
                        double zoomBottom,
                        double zoomFactor,
                        double zoomAspectRatio)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QMutexLocker l(&_imp->zoomCtxMutex);
    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
    Q_EMIT zoomChanged(100 * zoomFactor);
}

RectD
ViewerGL::getViewportRect() const
{
    RectD bbox;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        bbox.x1 = _imp->zoomCtx.left();
        bbox.y1 = _imp->zoomCtx.bottom();
        bbox.x2 = _imp->zoomCtx.right();
        bbox.y2 = _imp->zoomCtx.top();
    }

    return bbox;
}

void
ViewerGL::getCursorPosition(double &x,
                            double &y) const
{
    QPoint p = QCursor::pos();

    p = mapFromGlobal(p);
    QPointF mappedPos = toZoomCoordinates(p);
    x = mappedPos.x();
    y = mappedPos.y();
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
    if (QThread::currentThread() != qApp->thread()) {
        Q_EMIT mustCallUpdateOnMainThread();
    } else {
        update();
    }
}

void
ViewerGL::redrawNow()
{
    if (QThread::currentThread() != qApp->thread()) {
        Q_EMIT mustCallUpdateGLOnMainThread();
    } else {
        updateGL();
    }
}


void
ViewerGL::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    QGLFormat f = format();
    *hasAlpha = f.alpha();
    int r = f.redBufferSize();
    if (r == -1) {
        r = 8;// taken from qgl.h
    }
    int g = f.greenBufferSize();
    if (g == -1) {
        g = 8;// taken from qgl.h
    }
    int b = f.blueBufferSize();
    if (b == -1) {
        b = 8;// taken from qgl.h
    }
    int size = r;
    size = std::min(size, g);
    size = std::min(size, b);
    *depthPerComponents = size;
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

#ifdef OFX_EXTENSIONS_NATRON
double
ViewerGL::getScreenPixelRatio() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return windowHandle()->devicePixelRatio()
#else
    return 1.;
#endif
}
#endif

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

/**
 * @brief Returns the font height, i.e: the height of the highest letter for this font
 **/
int
ViewerGL::getWidgetFontHeight() const
{
    return fontMetrics().height();
}

/**
 * @brief Returns for a string the estimated pixel size it would take on the widget
 **/
int
ViewerGL::getStringWidthForCurrentFont(const std::string& string) const
{
    return fontMetrics().width( QString::fromUtf8( string.c_str() ) );
}


void
ViewerGL::removeGUI()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( _imp->viewerTab->getGui() ) {
        _imp->viewerTab->getGui()->removeViewerTab(_imp->viewerTab, true, true);
    }
}


ViewerNodePtr
ViewerGL::getInternalNode() const
{
    return _imp->viewerTab->getInternalNode();
}

ViewerTab*
ViewerGL::getViewerTab() const
{
    return _imp->viewerTab;
}

bool
ViewerGL::pickColorInternal(double x,
                            double y,
                            bool pickInput)
{

    float r, g, b, a;
    QPointF imgPos;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPos = _imp->zoomCtx.toZoomCoordinates(x, y);
    }

    ViewIdx currentView = getInternalNode()->getCurrentRenderView();

    _imp->lastPickerPos = imgPos;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool ret = false;
    for (int i = 0; i < 2; ++i) {
        // imgPos must be in canonical coordinates
        unsigned int mmLevel;
        bool picked = getColorAt(imgPos.x(), imgPos.y(), linear, i, pickInput, &r, &g, &b, &a, &mmLevel);
        if (picked) {
            if (i == 0) {
                _imp->viewerTab->getGui()->setColorPickersColor(currentView, r, g, b, a);
            }
            _imp->infoViewer[i]->setColorApproximated(mmLevel > 0);
            _imp->infoViewer[i]->setColorValid(true);
            if ( !_imp->infoViewer[i]->colorVisible() ) {
                _imp->infoViewer[i]->showColorInfo();
            }
            _imp->infoViewer[i]->setColor(r, g, b, a);
            ret = true;
            {
                ColorRgba<double> interactColor(r,g,b,a);
                setParametricParamsPickerColor(interactColor, true, true);
            }
        } else {
            _imp->infoViewer[i]->setColorValid(false);
            setParametricParamsPickerColor(ColorRgba<double>(), false, false);
        }
    }

    return ret;
}

// used for the ctrl-click color picker (not the information bar at the bottom of the viewer)
bool
ViewerGL::pickColor(double x,
                    double y,
                    bool pickInput)
{
    bool isSync = getInternalNode()->isViewersSynchroEnabled();

    if (isSync) {
        bool res = false;
        const std::list<ViewerTab*>& allViewers = _imp->viewerTab->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = allViewers.begin(); it != allViewers.end(); ++it) {
            bool ret = (*it)->getViewer()->pickColorInternal(x, y, pickInput);
            if ( (*it)->getViewer() == this ) {
                res = ret;
            }
        }

        return res;
    } else {
        return pickColorInternal(x, y, pickInput);
    }
}

void
ViewerGL::updateInfoWidgetColorPicker(const QPointF & imgPos,
                                      const QPoint & widgetPos)
{
    bool isDrawing = bool( _imp->viewerTab->getGui()->getApp()->getActiveRotoDrawingStroke() );


    if (!isDrawing) {
        for (int i = 0; i < 2; ++i) {
            const RectD& rod = getRoD(i);
            RectD canonicalDispW = getCanonicalFormat(i);
            updateInfoWidgetColorPickerInternal(imgPos, widgetPos, width(), height(), rod, canonicalDispW, i);
        }
    }
}

void
ViewerGL::updateInfoWidgetColorPickerInternal(const QPointF & imgPos,
                                              const QPoint & widgetPos,
                                              int width,
                                              int height,
                                              const RectD & rod, // in canonical coordinates
                                              const RectD & dispW, // in canonical coordinates
                                              int texIndex)
{
    if ( !_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->isGUIFrozen() ) {
        return;
    }

    const std::list<Histogram*>& histograms = _imp->viewerTab->getGui()->getHistograms();

    ViewerNodePtr viewerNode = getInternalNode();
    if ( _imp->displayTextures[texIndex].isVisible &&
         ( imgPos.x() >= rod.left() ) &&
         ( imgPos.x() < rod.right() ) &&
         ( imgPos.y() >= rod.bottom() ) &&
         ( imgPos.y() < rod.top() ) &&
         ( widgetPos.x() >= 0) && ( widgetPos.x() < width) &&
         ( widgetPos.y() >= 0) && ( widgetPos.y() < height) ) {
        ///if the clip to project format is enabled, make sure it is in the project format too
        if ( viewerNode->isClipToFormatEnabled() &&
             ( ( imgPos.x() < dispW.left() ) ||
               ( imgPos.x() >= dispW.right() ) ||
               ( imgPos.y() < dispW.bottom() ) ||
               ( imgPos.y() >= dispW.top() ) ) ) {
                 if ( _imp->infoViewer[texIndex]->colorVisible() && _imp->pickerState == ePickerStateInactive) {
                     _imp->infoViewer[texIndex]->hideColorInfo();
                 }
                 for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
                     if ( (*it)->getViewerTextureInputDisplayed() == texIndex ) {
                         (*it)->hideViewerCursor();
                     }
                 }
                 if (texIndex == 0 && _imp->pickerState == ePickerStateInactive) {
                     setParametricParamsPickerColor(ColorRgba<double>(), false, false);
                 }
             } else {
                 if (_imp->pickerState == ePickerStateInactive) {
                     updateColorPicker( texIndex, widgetPos.x(), widgetPos.y() );
                 } else if ( ( _imp->pickerState == ePickerStatePoint) || ( _imp->pickerState == ePickerStateRectangle) ) {
                     if ( !_imp->infoViewer[texIndex]->colorVisible() ) {
                         _imp->infoViewer[texIndex]->showColorInfo();
                     }
                 } else {
                     ///unkwn state
                     assert(false);
                 }
                 // Show the picker on parametric params without updating the color value
                 if (texIndex == 0) {
                     setParametricParamsPickerColor(ColorRgba<double>(), false, true);
                 }
             }
    } else {
        if ( _imp->infoViewer[texIndex]->colorVisible() && _imp->pickerState == ePickerStateInactive) {
            _imp->infoViewer[texIndex]->hideColorInfo();
        }
        for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
            if ( (*it)->getViewerTextureInputDisplayed() == texIndex ) {
                (*it)->hideViewerCursor();
            }
        }
        if (texIndex == 0 && _imp->pickerState == ePickerStateInactive) {
            setParametricParamsPickerColor(ColorRgba<double>(), false, false);
        }
    }

    double par;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        par = _imp->displayTextures[texIndex].pixelAspectRatio;
    }
    QPoint imgPosPixel;
    imgPosPixel.rx() = std::floor(imgPos.x() / par);
    imgPosPixel.ry() = std::floor( imgPos.y() );
    _imp->infoViewer[texIndex]->setMousePos(imgPosPixel);
    //if ( !_imp->infoViewer[texIndex]->mouseVisible() ) { // done in enterEvent
    //    _imp->infoViewer[texIndex]->showMouseInfo();
    //}
} // ViewerGL::updateInfoWidgetColorPickerInternal

void
ViewerGL::updateRectangleColorPicker()
{
    bool isSync = getInternalNode()->isViewersSynchroEnabled();

    if (isSync) {
        const std::list<ViewerTab*>& allViewers = _imp->viewerTab->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = allViewers.begin(); it != allViewers.end(); ++it) {
            (*it)->getViewer()->updateRectangleColorPickerInternal();
        }
    } else {
        updateRectangleColorPickerInternal();
    }
}

void
ViewerGL::updateRectangleColorPickerInternal()
{
    float r, g, b, a;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    QPointF topLeft = _imp->pickerRect.topLeft();
    QPointF btmRight = _imp->pickerRect.bottomRight();
    RectD rect;

    ViewIdx currentView = getInternalNode()->getCurrentRenderView();

    bool pickInput = (qApp->keyboardModifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier)) == (Qt::ControlModifier | Qt::AltModifier);

    rect.set_left( std::min( topLeft.x(), btmRight.x() ) );
    rect.set_right( std::max( topLeft.x(), btmRight.x() ) );
    rect.set_bottom( std::min( topLeft.y(), btmRight.y() ) );
    rect.set_top( std::max( topLeft.y(), btmRight.y() ) );
    for (int i = 0; i < 2; ++i) {
        unsigned int mm;
        bool picked = getColorAtRect(rect, linear, i, pickInput, &r, &g, &b, &a, &mm);
        if (picked) {
            if (i == 0) {
                _imp->viewerTab->getGui()->setColorPickersColor(currentView, r, g, b, a);
            }
            _imp->infoViewer[i]->setColorValid(true);
            if ( !_imp->infoViewer[i]->colorVisible() ) {
                _imp->infoViewer[i]->showColorInfo();
            }
            _imp->infoViewer[i]->setColorApproximated(mm > 0);
            _imp->infoViewer[i]->setColor(r, g, b, a);

            ColorRgba<double> c(r,g,b,a);
            setParametricParamsPickerColor(c, true, true);
        } else {
            _imp->infoViewer[i]->setColorValid(false);
            setParametricParamsPickerColor(ColorRgba<double>(), false, false);
        }
    }
}

void
ViewerGL::resetWipeControls()
{
    getInternalNode()->resetWipe();
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

void
ViewerGL::getTextureColorAt(int x,
                            int y,
                            double* r,
                            double *g,
                            double *b,
                            double *a)
{
    assert( QThread::currentThread() == qApp->thread() );
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(_imp->glContextWrapper);
    locker->attach();


    *r = 0;
    *g = 0;
    *b = 0;
    *a = 0;

    ImageBitDepthEnum bitDepth;

    {
        QMutexLocker k(&_imp->displayDataMutex);
        if (_imp->displayTextures[0].texture) {
            bitDepth = _imp->displayTextures[0].texture->getBitDepth();
        } else if (_imp->displayTextures[1].texture) {
            bitDepth = _imp->displayTextures[1].texture->getBitDepth();
        } else {
            return;
        }
    }

    QPointF pos;
    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        pos = _imp->zoomCtx.toWidgetCoordinates(x, y);
    }

    if (bitDepth == eImageBitDepthByte) {
        U32 pixel;
        GL_GPU::ReadBuffer(GL_FRONT);
        GL_GPU::ReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 red = 0, green = 0, blue = 0, alpha = 0;
        blue |= pixel;
        green |= (pixel >> 8);
        red |= (pixel >> 16);
        alpha |= (pixel >> 24);
        *r = (double)red * (1. / 255);
        *g = (double)green * (1. / 255);
        *b = (double)blue * (1. / 255);
        *a = (double)alpha * (1. / 255);
        glCheckError(GL_GPU);
    } else if (bitDepth == eImageBitDepthFloat) {
        GLfloat pixel[4];
        GL_GPU::ReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_RGBA, GL_FLOAT, pixel);
        *r = (double)pixel[0];
        *g = (double)pixel[1];
        *b = (double)pixel[2];
        *a = (double)pixel[3];
        glCheckError(GL_GPU);
    }
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

TimeLinePtr
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
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack(GL_GPU);
    GL_GPU::PushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack(GL_GPU);
    GL_GPU::PushClientAttrib(GL_ALL_ATTRIB_BITS);
    GL_GPU::MatrixMode(GL_PROJECTION);
    glCheckProjectionStack(GL_GPU);
    GL_GPU::PushMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    glCheckModelviewStack(GL_GPU);
    GL_GPU::PushMatrix();

    // set defaults to work around OFX plugin bugs
    GL_GPU::Enable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

// used by the RAII class OGLContextSaver
void
ViewerGL::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::BindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    GL_GPU::MatrixMode(GL_PROJECTION);
    GL_GPU::PopMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    GL_GPU::PopMatrix();
    GL_GPU::PopClientAttrib();
    GL_GPU::PopAttrib();
}


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif
int
ViewerGL::getMipMapLevelCombinedToZoomFactor() const
{
    if (!getInternalNode()) {
        return 0;
    }
    int mmLvl;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        mmLvl = _imp->displayTextures[0].mipMapLevel;
    }
    double factor = getZoomFactor();

    if (factor > 1) {
        factor = 1;
    }
    mmLvl = std::max( (double)mmLvl, -std::ceil(std::log(factor) / M_LN2) );

    return mmLvl;
}

unsigned int
ViewerGL::getCurrentRenderScale() const
{
    return getMipMapLevelCombinedToZoomFactor();
}

template <typename PIX, int maxValue>
static
bool
getColorAtSinglePixel(const Image::CPUData& image,
                   int x,
                   int y,             // in pixel coordinates
                   bool forceLinear,
                   const Color::Lut* srcColorSpace,
                   const Color::Lut* dstColorSpace,
                   float* r,
                   float* g,
                   float* b,
                   float* a)
{
    if ( !image.bounds.contains(x, y) ) {
        return false;
    }

    if (!image.ptrs[0]) {
        return false;
    }

    int pixelStride;
    const PIX* pix[4] = {NULL, NULL, NULL, NULL};
    Image::getChannelPointers<PIX>((const PIX**)image.ptrs, x, y, image.bounds, image.nComps, (PIX**)pix, &pixelStride);

    if (image.nComps >= 4) {
        *r = *pix[0] * (1.f / maxValue);
        *g = *pix[1] * (1.f / maxValue);
        *b = *pix[2] * (1.f / maxValue);
        *a = *pix[3] * (1.f / maxValue);
    } else if (image.nComps == 3) {
        *r = *pix[0] * (1.f / maxValue);
        *g = *pix[1] * (1.f / maxValue);
        *b = *pix[2] * (1.f / maxValue);
        *a = 1.;
    } else if (image.nComps == 2) {
        *r = *pix[0] * (1.f / maxValue);
        *g = *pix[1] * (1.f / maxValue);
        *b = 1.;
        *a = 1.;
    } else {
        *r = *g = *b = *a = *pix[0] * (1.f / maxValue);
    }

    // Convert to linear
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
                     bool pickInput, 
                     float* r,
                     float* g,
                     float* b,
                     float* a,
                     unsigned int* imgMmlevel)                               // output values
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);



    ImagePtr image;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        if (pickInput) {
            image = _imp->displayTextures[textureIndex].colorPickerInputImage;
        } else {
            image = _imp->displayTextures[textureIndex].colorPickerImage;
        }
        if (!image) {
            return false;
        }
    }
    
    ViewerColorSpaceEnum srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(image->getBitDepth());
    const Color::Lut* dstColorSpace;
    const Color::Lut* srcColorSpace;
    if ( (srcCS == _imp->displayingImageLut)
         && ( (_imp->displayingImageLut == eViewerColorSpaceLinear) || !forceLinear ) ) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        if ( image->getLayer().isColorPlane() ) {
            srcColorSpace = ViewerInstance::lutFromColorspace(srcCS);
            dstColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
        } else {
            srcColorSpace = dstColorSpace = 0;
        }
    }


    assert(image->getStorageMode() == eStorageModeRAM);


    Image::CPUData imageData;
    image->getCPUData(&imageData);


    *imgMmlevel = image->getMipMapLevel();
    double mipMapScale = 1. / ( 1 << *imgMmlevel );
    RenderScale scale = image->getProxyScale();
    scale.x *= mipMapScale;
    scale.y *= mipMapScale;
    double par;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        par = _imp->displayTextures[textureIndex].pixelAspectRatio;
    }

    ///Convert to pixel coords
    int xPixel = std::floor(x  * scale.x / par);
    int yPixel = std::floor(y * scale.y);
    bool gotval;
    switch (image->getBitDepth()) {
        case eImageBitDepthByte:
            gotval = getColorAtSinglePixel<unsigned char, 255>(imageData,
                                                               xPixel, yPixel,
                                                               forceLinear,
                                                               srcColorSpace,
                                                               dstColorSpace,
                                                               r, g, b, a);
            break;
        case eImageBitDepthShort:
            gotval = getColorAtSinglePixel<unsigned short, 65535>(imageData,
                                                                  xPixel, yPixel,
                                                                  forceLinear,
                                                                  srcColorSpace,
                                                                  dstColorSpace,
                                                                  r, g, b, a);
            break;
        case eImageBitDepthFloat:
            gotval = getColorAtSinglePixel<float, 1>(imageData,
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

template <typename PIX, int maxValue, int srcNComps>
static
bool
getColorAtRectForSrcNComps(const Image::CPUData& image,
                           const RectI& roi,
                           bool forceLinear,
                           const Color::Lut* srcColorSpace,
                           const Color::Lut* dstColorSpace,
                           double pixelSums[4])
{

    memset(pixelSums, 0, sizeof(double) * 4);

    U64 area = roi.area();

    int pixelStride;
    const PIX* pix[4] = {NULL, NULL, NULL, NULL};
    Image::getChannelPointers<PIX, srcNComps>((const PIX**)image.ptrs, roi.x1, roi.y1, image.bounds, (PIX**)pix, &pixelStride);


    for (int y = roi.y1; y < roi.y2; ++y) {

        for (int x = roi.x1; x < roi.x2; ++x) {

            float tmpPix[4] = {0., 0., 0., 0.};
            if (srcNComps == 1) {
                tmpPix[3] = Image::convertPixelDepth<PIX, float>(*pix[0]);
            } else {
                for (int c = 0; c < srcNComps; ++c) {
                    tmpPix[c] = Image::convertPixelDepth<PIX, float>(*pix[c]);
                }
            }

            // Convert to linear
            if (srcColorSpace && srcNComps > 1) {
                int nCompsToConvert = std::min(3, srcNComps);
                for (int c = 0; c < nCompsToConvert; ++c) {
                    tmpPix[c] = srcColorSpace->fromColorSpaceFloatToLinearFloat(tmpPix[c]);
                }
            }

            if (!forceLinear && dstColorSpace) {
                ///convert to dst color space
                float from[3];
                memcpy(from, tmpPix, sizeof(float) * 3);
                dstColorSpace->to_float_planar(tmpPix, from, 3);
            }


            for (int c = 0; c < 4; ++c) {
                pixelSums[c] += tmpPix[c];
                if (pix[c]) {
                    pix[c] += pixelStride;
                }
            }
        } // for each pixels along the line

        // Remove what was done on previous iteration and move to next line
        for (int c = 0; c < srcNComps; ++c) {
            pix[c] += ((image.bounds.width() - roi.width()) * pixelStride);
        }

    } // for each scan-line

    if (area > 0) {
        for (int c = 0; c < 4; ++c) {
            pixelSums[c] /= area;
        }
    }
    return true;


} // getColorAtRectForSrcNComps

template <typename PIX, int maxValue>
static
bool
getColorAtRectForDepth(const Image::CPUData& image,
                       const RectI& roi,
                       bool forceLinear,
                       const Color::Lut* srcColorSpace,
                       const Color::Lut* dstColorSpace,
                       double pixelSums[4])
{
    switch (image.nComps) {
        case 1:
            return getColorAtRectForSrcNComps<PIX, maxValue, 1>(image, roi, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
        case 2:
            return getColorAtRectForSrcNComps<PIX, maxValue, 2>(image, roi, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
        case 3:
            return getColorAtRectForSrcNComps<PIX, maxValue, 3>(image, roi, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
        case 4:
            return getColorAtRectForSrcNComps<PIX, maxValue, 4>(image, roi, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
        default:
            return false;
    }
} // getColorAtRectForDepth


bool
ViewerGL::getColorAtRect(const RectD &roi, // rectangle in canonical coordinates
                         bool forceLinear,
                         int textureIndex,
                         bool pickInput,
                         float* r,
                         float* g,
                         float* b,
                         float* a,
                         unsigned int* imgMmlevel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);



    ImagePtr image;

    {
        QMutexLocker k(&_imp->displayDataMutex);
        if (pickInput) {
            image = _imp->displayTextures[textureIndex].colorPickerInputImage;
        } else {
            image = _imp->displayTextures[textureIndex].colorPickerImage;
        }
        if (!image) {
            return false;
        }
    }


    Image::CPUData imageData;
    image->getCPUData(&imageData);
    
    ViewerColorSpaceEnum srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(image->getBitDepth());
    const Color::Lut* dstColorSpace;
    const Color::Lut* srcColorSpace;
    if ( (srcCS == _imp->displayingImageLut) && ( (_imp->displayingImageLut == eViewerColorSpaceLinear) || !forceLinear ) ) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        srcColorSpace = ViewerInstance::lutFromColorspace(srcCS);
        dstColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
    }

    *imgMmlevel = image->getMipMapLevel();
    double par;
    {
        QMutexLocker k(&_imp->displayDataMutex);
        par = _imp->displayTextures[textureIndex].pixelAspectRatio;
    }


    RectI roiPixels;
    roi.toPixelEnclosing(*imgMmlevel, par, &roiPixels);

    double pixelSums[4];
    switch (image->getBitDepth()) {
        case eImageBitDepthByte:
            getColorAtRectForDepth<unsigned char, 255>(imageData, roiPixels, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
            break;
        case eImageBitDepthShort:
            getColorAtRectForDepth<unsigned short, 65535>(imageData, roiPixels, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
            break;
        case eImageBitDepthFloat:
            getColorAtRectForDepth<float, 1>(imageData, roiPixels, forceLinear, srcColorSpace, dstColorSpace, pixelSums);
            break;
        case eImageBitDepthHalf:
        case eImageBitDepthNone:
            break;
    }

    *r = pixelSums[0];
    *g = pixelSums[1];
    *b = pixelSums[2];
    *a = pixelSums[3];




    return false;
} // getColorAtRect

TimeValue
ViewerGL::getCurrentlyDisplayedTime() const
{

    QMutexLocker k(&_imp->displayDataMutex);
    if (_imp->displayTextures[0].isVisible) {
        return _imp->displayTextures[0].time;
    } else {
        return TimeValue(_imp->viewerTab->getTimeLine()->currentFrame());
    }
}

void
ViewerGL::getViewerFrameRange(int* first,
                              int* last) const
{
    _imp->viewerTab->getTimelineBounds(first, last);
}

double
ViewerGL::currentTimeForEvent(QInputEvent* e)
{
#if QT_VERSION >= 0x050000
    // timestamp() is usually in milliseconds
    if ( e->timestamp() ) {
        return (double)e->timestamp() / 1000000;
    }
#else
    Q_UNUSED(e);
#endif
    // Qt 4 has no event timestamp, use gettimeofday (defined in Timer.cpp for windows)
    struct timeval now;
    gettimeofday(&now, 0);

    return now.tv_sec + now.tv_usec / 1000000.0;
}


void
ViewerGL::setInfoBarVisible(bool visible)
{
    getViewerTab()->setInfobarVisible(visible);
}

void
ViewerGL::setInfoBarVisible(int index, bool visible)
{
    getViewerTab()->setInfobarVisible(index, visible);
}

void
ViewerGL::setLeftToolBarVisible(bool visible)
{
    getViewerTab()->setLeftToolbarVisible(visible);
}

void
ViewerGL::setTopToolBarVisible(bool visible)
{
    getViewerTab()->setTopToolbarVisible(visible);
}


void
ViewerGL::setPlayerVisible(bool visible)
{
    getViewerTab()->setPlayerVisible(visible);
}


void
ViewerGL::setTimelineVisible(bool visible)
{
    getViewerTab()->setTimelineVisible(visible);
}

void
ViewerGL::setTabHeaderVisible(bool visible)
{
    getViewerTab()->setTabHeaderVisible(visible);
}

void
ViewerGL::setTimelineBounds(double first, double last)
{
    getViewerTab()->setTimelineBounds(first, last);
}

void
ViewerGL::setTimelineFormatFrames(bool value)
{
    getViewerTab()->setTimelineFormatFrames(value);

}

void
ViewerGL::setTripleSyncEnabled(bool toggled)
{
    getViewerTab()->setTripleSyncEnabled(toggled);
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_ViewerGL.cpp"

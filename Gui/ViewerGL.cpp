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

#include "ViewerGL.h"
#include "ViewerGLPrivate.h"

#include <cassert>
#include <algorithm> // min, max
#include <cstring> // for std::memcpy, std::memset, std::strcmp, std::strchr
#include <stdexcept>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

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
#include <QtOpenGL/QGLShaderProgram>
#include <QTreeWidget>
#include <QTabBar>

#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/NodeGuiI.h"
#include "Engine/Project.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/KnobTypes.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h" // for gettimeofday
#include "Engine/Texture.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h" // kShortcutGroupViewer ...
#include "Gui/CurveWidget.h"
#include "Gui/Gui.h"
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

NATRON_NAMESPACE_ENTER;


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

    QObject::connect( parent->getGui()->getApp()->getProject().get(), SIGNAL(formatChanged(Format)), this, SLOT(onProjectFormatChanged(Format)) );
    Format projectFormat;
    parent->getGui()->getApp()->getProject()->getProjectDefaultFormat(&projectFormat);

    RectD canonicalFormat = projectFormat.toCanonicalFormat();

    for (int i = 0; i < 2; ++i) {
        setRegionOfDefinition(canonicalFormat, projectFormat.getPixelAspectRatio(), i);
    }
    onProjectFormatChangedInternal(projectFormat, false);

    QObject::connect( appPTR, SIGNAL(checkerboardSettingsChanged()), this, SLOT(onCheckerboardSettingsChanged()) );
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
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->displayTextures[0].isVisible || _imp->displayTextures[1].isVisible;
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
    GL_GPU::glViewport (0, 0, w, h);
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
    ViewerInstancePtr viewer = _imp->viewerTab->getInternalNode()->getInternalViewerNode();
    if (!viewer) {
        return;
    }

    bool isLoadingProject = _imp->viewerTab->getGui() && _imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject();
    bool canResize = _imp->viewerTab->getGui() && !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject() &&  !_imp->viewerTab->getGui()->getApp()->isDuringPainting();

    if (!zoomSinceLastFit && canResize) {
        fitImageToFormat();
    }
    if ( viewer->getUiContext() && !isLoadingProject  &&
         ( ( oldWidth != w) || ( oldHeight != h) ) ) {
        viewer->renderCurrentFrame(true);

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
            GL_GPU::glEnable(GL_BLEND);
        }
        switch (premult) {
        case eImagePremultiplicationPremultiplied:
            GL_GPU::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case eImagePremultiplicationUnPremultiplied:
            GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case eImagePremultiplicationOpaque:
            break;
        }
    }

    ~BlendSetter()
    {
        if (didBlend) {
            GL_GPU::glDisable(GL_BLEND);
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


        GL_GPU::glMatrixMode(GL_PROJECTION);
        GL_GPU::glLoadIdentity();
        GL_GPU::glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, -1, 1);
        _imp->glShadow.setX( (zoomRight - zoomLeft) / width() );
        _imp->glShadow.setY( (zoomTop - zoomBottom) / height() );
        //glScalef(RoD.width, RoD.width, 1.0); // for compatibility with Nuke
        //glTranslatef(1, 1, 0);     // for compatibility with Nuke
        GL_GPU::glMatrixMode(GL_MODELVIEW);
        GL_GPU::glLoadIdentity();
        //glTranslatef(-1, -1, 0);        // for compatibility with Nuke
        //glScalef(1/RoD.width, 1./RoD.width, 1.0); // for compatibility with Nuke

        glCheckError(GL_GPU);


        ViewerNodePtr viewerNode = _imp->viewerTab->getInternalNode();
        if (!viewerNode) {
            return;
        }
        // don't even bind the shader on 8-bits gamma-compressed textures
        ViewerCompositingOperatorEnum compOperator = viewerNode->getCurrentOperator();

        ///Determine whether we need to draw each texture or not
        ViewerInstancePtr internalViewer = viewerNode->getInternalViewerNode();
        if (!internalViewer || !internalViewer->getNode()) {
            return;
        }


        NodePtr aInputNode = viewerNode->getCurrentAInput();
        NodePtr bInputNode = viewerNode->getCurrentBInput();

        bool drawTexture[2];
        drawTexture[0] = _imp->displayTextures[0].isVisible;
        drawTexture[1] = _imp->displayTextures[1].isVisible && compOperator != eViewerCompositingOperatorNone;
        if ( (aInputNode == bInputNode) &&
             (compOperator != eViewerCompositingOperatorWipeMinus) &&
             (compOperator != eViewerCompositingOperatorStackMinus) ) {
            drawTexture[1] = false;
        }

        double wipeMix = viewerNode->getWipeAmount();
    
        GLuint savedTexture;
        bool stack = (compOperator == eViewerCompositingOperatorStackUnder ||
                      compOperator == eViewerCompositingOperatorStackOver ||
                      compOperator == eViewerCompositingOperatorStackMinus ||
                      compOperator == eViewerCompositingOperatorStackOnionSkin ||
                      false);

        GL_GPU::glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
        {
            GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

            clearColorBuffer( _imp->clearColor.redF(), _imp->clearColor.greenF(), _imp->clearColor.blueF(), _imp->clearColor.alphaF() );
            glCheckErrorIgnoreOSXBug(GL_GPU);

            GL_GPU::glEnable (GL_TEXTURE_2D);
            GL_GPU::glColor4d(1., 1., 1., 1.);
            GL_GPU::glBlendColor(1, 1, 1, wipeMix);

            bool checkerboard = viewerNode->isCheckerboardEnabled();
            if (checkerboard) {
                // draw checkerboard texture, but only on the left side if in wipe mode
                RectD projectFormatCanonical;
                _imp->getProjectFormatCanonical(projectFormatCanonical);
                if (compOperator == eViewerCompositingOperatorNone) {
                    _imp->drawCheckerboardTexture(projectFormatCanonical);
                } else if ( operatorIsWipe(compOperator) ) {
                    QPolygonF polygonPoints;
                    Implementation::WipePolygonEnum t = _imp->getWipePolygon(projectFormatCanonical,
                                                                             false,
                                                                             &polygonPoints);
                    if (t == Implementation::eWipePolygonFull) {
                        _imp->drawCheckerboardTexture(projectFormatCanonical);
                    } else if (t == Implementation::eWipePolygonPartial) {
                        _imp->drawCheckerboardTexture(polygonPoints);
                    }
                }
            }


            ///Depending on the premultiplication of the input image we use a different blending func
            ImagePremultiplicationEnum premultA = _imp->displayTextures[0].premult;

            // Left side of the wipe is displayed as Opaque if there is no checkerboard.
            // That way, unpremultiplied images can easily be displayed, even if their alpha is zero.
            // We do not "unpremult" premultiplied RGB for displaying it, because it is the usual way
            // to visualize masks: areas with alpha=0 appear as black.
            switch (compOperator) {
            case eViewerCompositingOperatorNone: {
                if (drawTexture[0]) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, eDrawPolygonModeWhole, true);
                }
                break;
            }
            case eViewerCompositingOperatorWipeUnder:
            case eViewerCompositingOperatorStackUnder: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }
                if (drawTexture[1]) {
                    GL_GPU::glEnable(GL_BLEND);
                    GL_GPU::glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(_imp->displayTextures[1].mipMapLevel, 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::glDisable(GL_BLEND);
                }

                break;
            }
            case eViewerCompositingOperatorWipeOver:
            case eViewerCompositingOperatorStackOver: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[1]) {
                    GL_GPU::glEnable(GL_BLEND);
                    GL_GPU::glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(_imp->displayTextures[1].mipMapLevel, 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::glDisable(GL_BLEND);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }

                break;
            }
            case eViewerCompositingOperatorWipeMinus:
            case eViewerCompositingOperatorStackMinus: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }
                if (drawTexture[1]) {
                    GL_GPU::glEnable(GL_BLEND);
                    GL_GPU::glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                    GL_GPU::glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                    _imp->drawRenderingVAO(_imp->displayTextures[1].mipMapLevel, 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::glDisable(GL_BLEND);
                }
                break;
            }
            case eViewerCompositingOperatorWipeOnionSkin:
            case eViewerCompositingOperatorStackOnionSkin: {
                if (drawTexture[0] && !stack) {
                    BlendSetter b(checkerboard ? premultA : eImagePremultiplicationOpaque);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, eDrawPolygonModeWipeLeft, true);
                }
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayTextures[0].mipMapLevel, 0, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                }
                if (drawTexture[1]) {
                    GL_GPU::glEnable(GL_BLEND);
                    GL_GPU::glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                    //glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                    _imp->drawRenderingVAO(_imp->displayTextures[1].mipMapLevel, 1, stack ? eDrawPolygonModeWhole : eDrawPolygonModeWipeRight, false);
                    GL_GPU::glDisable(GL_BLEND);
                }
                break;
            }
            } // switch

            for (std::size_t i = 0; i < _imp->partialUpdateTextures.size(); ++i) {
                const TextureRect &r = _imp->partialUpdateTextures[i].texture->getTextureRect();
                RectI texRect(r.x1, r.y1, r.x2, r.y2);
                const double par = r.par;
                RectD canonicalTexRect;
                texRect.toCanonical_noClipping(_imp->partialUpdateTextures[i].mipMapLevel, par /*, rod*/, &canonicalTexRect);

                GL_GPU::glActiveTexture(GL_TEXTURE0);
                GL_GPU::glBindTexture( GL_TEXTURE_2D, _imp->partialUpdateTextures[i].texture->getTexID() );
                GL_GPU::glBegin(GL_POLYGON);
                GL_GPU::glTexCoord2d(0, 0); GL_GPU::glVertex2d(canonicalTexRect.x1, canonicalTexRect.y1);
                GL_GPU::glTexCoord2d(0, 1); GL_GPU::glVertex2d(canonicalTexRect.x1, canonicalTexRect.y2);
                GL_GPU::glTexCoord2d(1, 1); GL_GPU::glVertex2d(canonicalTexRect.x2, canonicalTexRect.y2);
                GL_GPU::glTexCoord2d(1, 0); GL_GPU::glVertex2d(canonicalTexRect.x2, canonicalTexRect.y1);
                GL_GPU::glEnd();
                GL_GPU::glBindTexture(GL_TEXTURE_2D, 0);

                glCheckError(GL_GPU);
            }
        } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

        ///Unbind render textures for overlays
        GL_GPU::glBindTexture(GL_TEXTURE_2D, savedTexture);

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

        GL_GPU::glClearColor(r, g, b, a);
        GL_GPU::glClear(GL_COLOR_BUFFER_BIT);
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

    RectD projectFormatCanonical;
    _imp->getProjectFormatCanonical(projectFormatCanonical);
    renderText(projectFormatCanonical.right(), projectFormatCanonical.bottom(), _imp->currentViewerInfo_resolutionOverlay, _imp->textRenderingColor, _imp->textFont);


    QPoint topRight( projectFormatCanonical.right(), projectFormatCanonical.top() );
    QPoint topLeft( projectFormatCanonical.left(), projectFormatCanonical.top() );
    QPoint btmLeft( projectFormatCanonical.left(), projectFormatCanonical.bottom() );
    QPoint btmRight( projectFormatCanonical.right(), projectFormatCanonical.bottom() );

    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        GL_GPU::glDisable(GL_BLEND);

        GL_GPU::glBegin(GL_LINES);

        GL_GPU::glColor4f( _imp->displayWindowOverlayColor.redF(),
                   _imp->displayWindowOverlayColor.greenF(),
                   _imp->displayWindowOverlayColor.blueF(),
                   _imp->displayWindowOverlayColor.alphaF() );
        GL_GPU::glVertex3f(btmRight.x(), btmRight.y(), 1);
        GL_GPU::glVertex3f(btmLeft.x(), btmLeft.y(), 1);

        GL_GPU::glVertex3f(btmLeft.x(), btmLeft.y(), 1);
        GL_GPU::glVertex3f(topLeft.x(), topLeft.y(), 1);

        GL_GPU::glVertex3f(topLeft.x(), topLeft.y(), 1);
        GL_GPU::glVertex3f(topRight.x(), topRight.y(), 1);

        GL_GPU::glVertex3f(topRight.x(), topRight.y(), 1);
        GL_GPU::glVertex3f(btmRight.x(), btmRight.y(), 1);

        GL_GPU::glEnd();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        ViewerNodePtr viewerNode = _imp->viewerTab->getInternalNode();
        if (!viewerNode) {
            return;
        }
        ViewerCompositingOperatorEnum compOperator = viewerNode->getCurrentOperator();


        for (int i = 0; i < 2; ++i) {
            if ( !_imp->displayTextures[i].isVisible || !bInput) {
                continue;
            }
            if ( (i == 1) && (compOperator == eViewerCompositingOperatorNone) ) {
                break;
            }
            RectD dataW = getRoD(i);

            if (dataW != projectFormatCanonical) {
                renderText(dataW.right(), dataW.top(),
                           _imp->currentViewerInfo_topRightBBOXoverlay[i], _imp->rodOverlayColor, _imp->textFont);
                renderText(dataW.left(), dataW.bottom(),
                           _imp->currentViewerInfo_btmLeftBBOXoverlay[i], _imp->rodOverlayColor, _imp->textFont);
                glCheckError(GL_GPU);

                QPointF topRight2( dataW.right(), dataW.top() );
                QPointF topLeft2( dataW.left(), dataW.top() );
                QPointF btmLeft2( dataW.left(), dataW.bottom() );
                QPointF btmRight2( dataW.right(), dataW.bottom() );
                GL_GPU::glLineStipple(2, 0xAAAA);
                GL_GPU::glEnable(GL_LINE_STIPPLE);
                GL_GPU::glBegin(GL_LINES);
                GL_GPU::glColor4f( _imp->rodOverlayColor.redF(),
                           _imp->rodOverlayColor.greenF(),
                           _imp->rodOverlayColor.blueF(),
                           _imp->rodOverlayColor.alphaF() );
                GL_GPU::glVertex3f(btmRight2.x(), btmRight2.y(), 1);
                GL_GPU::glVertex3f(btmLeft2.x(), btmLeft2.y(), 1);

                GL_GPU::glVertex3f(btmLeft2.x(), btmLeft2.y(), 1);
                GL_GPU::glVertex3f(topLeft2.x(), topLeft2.y(), 1);

                GL_GPU::glVertex3f(topLeft2.x(), topLeft2.y(), 1);
                GL_GPU::glVertex3f(topRight2.x(), topRight2.y(), 1);

                GL_GPU::glVertex3f(topRight2.x(), topRight2.y(), 1);
                GL_GPU::glVertex3f(btmRight2.x(), btmRight2.y(), 1);
                GL_GPU::glEnd();
                GL_GPU::glDisable(GL_LINE_STIPPLE);
                glCheckErrorIgnoreOSXBug(GL_GPU);
            }
        }


        glCheckError(GL_GPU);
        GL_GPU::glColor4f(1., 1., 1., 1.);
        double scale = 1. / (1 << mipMapLevel);

        /*
           Draw the overlays corresponding to the image displayed on the viewer, not the current timeline's time
         */
        double time = getCurrentlyDisplayedTime();
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

        GL_GPU::glColor3f(0.9, 0.7, 0.);
        QPointF topLeft = _imp->pickerRect.topLeft();
        QPointF btmRight = _imp->pickerRect.bottomRight();
        ///base rect
        GL_GPU::glBegin(GL_LINE_LOOP);
        GL_GPU::glVertex2f( topLeft.x(), btmRight.y() ); //bottom left
        GL_GPU::glVertex2f( topLeft.x(), topLeft.y() ); //top left
        GL_GPU::glVertex2f( btmRight.x(), topLeft.y() ); //top right
        GL_GPU::glVertex2f( btmRight.x(), btmRight.y() ); //bottom right
        GL_GPU::glEnd();
    } // GLProtectAttrib a(GL_CURRENT_BIT);
}

void
ViewerGL::drawPickerPixel()
{
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::glEnable(GL_POINT_SMOOTH);
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            GL_GPU::glPointSize( 1. * _imp->zoomCtx.factor() );
        }

        QPointF pos = _imp->lastPickerPos;
        unsigned int mipMapLevel = _imp->displayTextures[0].mipMapLevel;

        if (mipMapLevel != 0) {
            pos *= (1 << mipMapLevel);
        }
        GL_GPU::glColor3f(0.9, 0.7, 0.);
        GL_GPU::glBegin(GL_POINTS);
        GL_GPU::glVertex2d( pos.x(), pos.y() );
        GL_GPU::glEnd();
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

        GL_GPU::glDisable(GL_BLEND);

        if (_imp->persistentMessageType == 1) { // error
            GL_GPU::glColor4f(0.5, 0., 0., 1.);
        } else { // warning
            GL_GPU::glColor4f(0.65, 0.65, 0., 1.);
        }
        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f( topLeft.x(), topLeft.y() ); //top left
        GL_GPU::glVertex2f( topLeft.x(), bottomRight.y() ); //bottom left
        GL_GPU::glVertex2f( bottomRight.x(), bottomRight.y() ); //bottom right
        GL_GPU::glVertex2f( bottomRight.x(), topLeft.y() ); //top right
        GL_GPU::glEnd();


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
    makeCurrent();
    if ( !appPTR->isOpenGLLoaded() ) {
        throw std::runtime_error("OpenGL was not loaded");
    }
    _imp->initializeGL();
}

GLuint
ViewerGL::getPboID(int index)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( index >= (int)_imp->pboIds.size() ) {
        GLuint handle;
        GL_GPU::glGenBuffers(1, &handle);
        _imp->pboIds.push_back(handle);

        return handle;
    } else {
        return _imp->pboIds[index];
    }
}

ViewerInstancePtr
ViewerGL::getInternalViewerNode() const
{
    return getInternalNode()->getInternalViewerNode();
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
                                     const double par,
                                     unsigned int mipMapLevel)
{
    // MT-SAFE
    RectD visibleArea;
    RectI ret;
    ViewerNodePtr viewerNode = getInternalNode();
    if (viewerNode->isFullFrameProcessingEnabled()) {
        ret = imageRoDPixel;
    } else {
        {
            QMutexLocker l(&_imp->zoomCtxMutex);
            QPointF topLeft =  _imp->zoomCtx.toZoomCoordinates(0, 0);
            visibleArea.x1 =  topLeft.x();
            visibleArea.y2 =  topLeft.y();
            QPointF bottomRight = _imp->zoomCtx.toZoomCoordinates(width() - 1, height() - 1);
            visibleArea.x2 = bottomRight.x();
            visibleArea.y1 = bottomRight.y();
        }

        if (mipMapLevel != 0) {
            // for the viewer, we need the smallest enclosing rectangle at the mipmap level, in order to avoid black borders
            visibleArea.toPixelEnclosing(mipMapLevel, par, &ret);
        } else {
            ret.x1 = std::floor(visibleArea.x1 / par);
            ret.x2 = std::ceil(visibleArea.x2 / par);
            ret.y1 = std::floor(visibleArea.y1);
            ret.y2 = std::ceil(visibleArea.y2);
        }

        ///If the roi doesn't intersect the image's Region of Definition just return an empty rectangle
        if ( !ret.intersect(imageRoDPixel, &ret) ) {
            ret.clear();
        }

    }

    ///to clip against the user roi however clip it against the mipmaplevel of the zoomFactor+proxy

    RectD userRoI = viewerNode->getUserRoI();
    bool userRoiEnabled = viewerNode->isUserRoIEnabled();

    if (userRoiEnabled) {
        RectI userRoIpixel;

        ///If the user roi is enabled, we want to render the smallest enclosing rectangle in order to avoid black borders.
        userRoI.toPixelEnclosing(mipMapLevel, par, &userRoIpixel);

        ///If the user roi doesn't intersect the actually visible portion on the viewer, return an empty rectangle.
        if ( !ret.intersect(userRoIpixel, &ret) ) {
            ret.clear();
        }
    }

    return ret;
} // ViewerGL::getImageRectangleDisplayed

RectI
ViewerGL::getExactImageRectangleDisplayed(const RectD & rod,
                                          const double par,
                                          unsigned int mipMapLevel)
{

    bool clipToProject = getInternalNode()->isClipToProjectEnabled();
    RectD clippedRod;

    if (clipToProject) {
        RectD projectFormatCanonical;
        _imp->getProjectFormatCanonical(projectFormatCanonical);
        rod.intersect(projectFormatCanonical, &clippedRod);
    } else {
        clippedRod = rod;
    }

    RectI bounds;
    clippedRod.toPixelEnclosing(mipMapLevel, par, &bounds);
    RectI roi = getImageRectangleDisplayed(bounds, par, mipMapLevel);

    return roi;
}

RectI
ViewerGL::getImageRectangleDisplayedRoundedToTileSize(const RectD & rod,
                                                      const double par,
                                                      unsigned int mipMapLevel,
                                                      std::vector<RectI>* tiles,
                                                      std::vector<RectI>* tilesRounded,
                                                      int *viewerTileSize,
                                                      RectI* roiNotRounded)
{
    bool clipToProject = getInternalNode()->isClipToProjectEnabled();
    RectD clippedRod;

    if (clipToProject) {
        RectD projectFormatCanonical;
        _imp->getProjectFormatCanonical(projectFormatCanonical);
        rod.intersect(projectFormatCanonical, &clippedRod);
    } else {
        clippedRod = rod;
    }

    RectI bounds;
    clippedRod.toPixelEnclosing(mipMapLevel, par, &bounds);
    RectI roi = getImageRectangleDisplayed(bounds, par, mipMapLevel);

    ////Texrect is the coordinates of the 4 corners of the texture in the bounds with the current zoom
    ////factor taken into account.
    RectI texRect;
    int tileSize = std::pow( 2., (double)appPTR->getCurrentSettings()->getViewerTilesPowerOf2() );
    texRect.x1 = std::floor( ( (double)roi.x1 ) / tileSize ) * (double)tileSize;
    texRect.y1 = std::floor( ( (double)roi.y1 ) / tileSize ) * (double)tileSize;
    texRect.x2 = std::ceil( ( (double)roi.x2 ) / tileSize ) * (double)tileSize;
    texRect.y2 = std::ceil( ( (double)roi.y2 ) / tileSize ) * (double)tileSize;

    // Make sure the bounds of the area to render in the texture lies in the bounds
    if (roiNotRounded) {
        *roiNotRounded = roi;
    }
    if (tilesRounded) {
        for (int y = texRect.y1; y < texRect.y2; y += tileSize) {
            int y2 = std::min(y + tileSize, texRect.y2);
            for (int x = texRect.x1; x < texRect.x2; x += tileSize) {
                tilesRounded->resize(tilesRounded->size() + 1);
                RectI& tile = tilesRounded->back();
                tile.x1 = x;
                tile.x2 = std::min(x + tileSize, texRect.x2);
                tile.y1 = y;
                tile.y2 = y2;

                if (tiles) {
                    RectI tileRectRounded;
                    tile.intersect(bounds, &tileRectRounded);
                    tiles->push_back(tileRectRounded);
                }
                assert( texRect.contains(tile) );
            }
        }
    }

    if (viewerTileSize) {
        *viewerTileSize = tileSize;
    }
    return texRect;
}

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
    extensions = GL_GPU::glGetString(GL_EXTENSIONS);
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
ViewerGL::initShaderGLSL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if (!_imp->shaderLoaded) {
        _imp->shaderBlack.reset( new QGLShaderProgram( context() ) );
        if ( !_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Vertex, vertRGB) ) {
            qDebug() << qPrintable( _imp->shaderBlack->log() );
        }
        if ( !_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Fragment, blackFrag) ) {
            qDebug() << qPrintable( _imp->shaderBlack->log() );
        }
        if ( !_imp->shaderBlack->link() ) {
            qDebug() << qPrintable( _imp->shaderBlack->log() );
        }

        _imp->shaderRGB.reset( new QGLShaderProgram( context() ) );
        if ( !_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Vertex, vertRGB) ) {
            qDebug() << qPrintable( _imp->shaderRGB->log() );
        }
        if ( !_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Fragment, fragRGB) ) {
            qDebug() << qPrintable( _imp->shaderRGB->log() );
        }

        if ( !_imp->shaderRGB->link() ) {
            qDebug() << qPrintable( _imp->shaderRGB->log() );
        }
        _imp->shaderLoaded = true;
    }
}

void
ViewerGL::clearPartialUpdateTextures()
{
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
ViewerGL::endTransferBufferFromRAMToGPU(int textureIndex,
                                        const boost::shared_ptr<Texture>& texture,
                                        const ImagePtr& image,
                                        int time,
                                        const RectD& rod,
                                        double par,
                                        ImageBitDepthEnum depth,
                                        unsigned int mipMapLevel,
                                        ImagePremultiplicationEnum premult,
                                        double /*gain*/,
                                        double /*gamma*/,
                                        double offset,
                                        int lut,
                                        bool recenterViewer,
                                        const Point& viewportCenter,
                                        bool isPartialRect)
{
    if (recenterViewer) {
        QMutexLocker k(&_imp->zoomCtxMutex);
        double curCenterX = ( _imp->zoomCtx.left() + _imp->zoomCtx.right() ) / 2.;
        double curCenterY = ( _imp->zoomCtx.bottom() + _imp->zoomCtx.top() ) / 2.;
        _imp->zoomCtx.translate(viewportCenter.x - curCenterX, viewportCenter.y - curCenterY);
    }

    if (isPartialRect) {
        TextureInfo info;
        info.texture = boost::dynamic_pointer_cast<Texture>(texture);
        info.offset = offset;
        info.mipMapLevel = mipMapLevel;
        info.premult = premult;
        info.time = time;
        info.memoryHeldByLastRenderedImages = 0;
        info.isPartialImage = true;
        info.isVisible = true;
        _imp->partialUpdateTextures.push_back(info);
        // Update time otherwise overlays won't refresh
        _imp->displayTextures[0].time = time;
        _imp->displayTextures[1].time = time;
    } else {
        ViewerNodePtr internalNode = getInternalNode();
        _imp->displayTextures[textureIndex].isVisible = true;
        _imp->displayTextures[textureIndex].offset = offset;
        _imp->displayTextures[textureIndex].mipMapLevel = mipMapLevel;
        _imp->displayingImageLut = (ViewerColorSpaceEnum)lut;
        _imp->displayTextures[textureIndex].premult = premult;
        _imp->displayTextures[textureIndex].time = time;

        if (_imp->displayTextures[textureIndex].memoryHeldByLastRenderedImages > 0) {
            internalNode->unregisterPluginMemory(_imp->displayTextures[textureIndex].memoryHeldByLastRenderedImages);
            _imp->displayTextures[textureIndex].memoryHeldByLastRenderedImages = 0;
        }


        if (image) {
            _imp->viewerTab->setImageFormat(textureIndex, image->getComponents(), depth);
            RectI pixelRoD;
            image->getRoD().toPixelEnclosing(0, image->getPixelAspectRatio(), &pixelRoD);
            {
                QMutexLocker k(&_imp->projectFormatMutex);
                _imp->displayTextures[textureIndex].format = Format( _imp->projectFormat, image->getPixelAspectRatio() );
            }
            {
                QMutexLocker k(&_imp->lastRenderedImageMutex);
                _imp->displayTextures[textureIndex].lastRenderedTiles[mipMapLevel] = image;
            }
            _imp->displayTextures[textureIndex].memoryHeldByLastRenderedImages = 0;
            _imp->displayTextures[textureIndex].memoryHeldByLastRenderedImages += image->size();

            internalNode->registerPluginMemory(_imp->displayTextures[textureIndex].memoryHeldByLastRenderedImages);
            Q_EMIT imageChanged(textureIndex, true);
        } else {
            if ( !_imp->displayTextures[textureIndex].lastRenderedTiles[mipMapLevel] ) {
                Q_EMIT imageChanged(textureIndex, false);
            } else {
                Q_EMIT imageChanged(textureIndex, true);
            }
        }
        setRegionOfDefinition(rod, par, textureIndex);
    }
} // ViewerGL::endTransferBufferFromRAMToGPU

void
ViewerGL::transferBufferFromRAMtoGPU(const unsigned char* ramBuffer,
                                     size_t bytesCount,
                                     const RectI &roiRoundedToTileSize,
                                     const RectI& roi,
                                     const TextureRect & tileRect,
                                     int textureIndex,
                                     bool isPartialRect,
                                     bool isFirstTile,
                                     boost::shared_ptr<Texture>* texture)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    GLenum e = GL_GPU::glGetError();
    Q_UNUSED(e);

    GLint currentBoundPBO = 0;
    GL_GPU::glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &currentBoundPBO);
    GLenum err = GL_GPU::glGetError();
    if ( (err != GL_NO_ERROR) || (currentBoundPBO != 0) ) {
        qDebug() << "(ViewerGL::allocateAndMapPBO): Another PBO is currently mapped, glMap failed.";
    }

    // We use 2 PBOs to make use of asynchronous data uploading
    GLuint pboId = getPboID(_imp->updateViewerPboIndex);

    // The bitdepth of the texture
    ImageBitDepthEnum bd = getBitDepth();
    Texture::DataTypeEnum dataType;
    if (bd == eImageBitDepthByte) {
        dataType = Texture::eDataTypeByte;
    } else {
        //do 32bit fp textures either way, don't bother with half float. We might support it further on.
        dataType = Texture::eDataTypeFloat;
    }
    assert(textureIndex == 0 || textureIndex == 1);

    GLTexturePtr tex;
    TextureRect textureRectangle;
    if (isPartialRect) {
        // For small partial updates overlays, we make new textures
        int format, internalFormat, glType;

        if (dataType == Texture::eDataTypeFloat) {
            Texture::getRecommendedTexParametersForRGBAFloatTexture(&format, &internalFormat, &glType);
        } else {
            Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
        }
        tex.reset( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, dataType, format, internalFormat, glType, true) );
        textureRectangle = tileRect;
    } else {
        // re-use the existing texture if possible
        tex = _imp->displayTextures[textureIndex].texture;
        if (tex->type() != dataType) {
            int format, internalFormat, glType;

            if (dataType == Texture::eDataTypeFloat) {
                Texture::getRecommendedTexParametersForRGBAFloatTexture(&format, &internalFormat, &glType);
            } else {
                Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
            }
            _imp->displayTextures[textureIndex].texture.reset( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, dataType, format, internalFormat, glType, true) );
        }
        textureRectangle.set(roiRoundedToTileSize);
        _imp->displayTextures[textureIndex].roiNotRoundedToTileSize.set(roi);
        _imp->displayTextures[textureIndex].roiNotRoundedToTileSize.closestPo2 = tileRect.closestPo2;
        _imp->displayTextures[textureIndex].roiNotRoundedToTileSize.par = tileRect.par;
        textureRectangle.par = tileRect.par;
        textureRectangle.closestPo2 = tileRect.closestPo2;
        if (isFirstTile) {
            tex->ensureTextureHasSize(textureRectangle, 0);
        }
    }

    // bind PBO to update texture source
    GL_GPU::glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboId );

    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // until GPU to finish its job. To avoid waiting (idle), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    GL_GPU::glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);

    // map the buffer object into client's memory
    GLvoid *ret = GL_GPU::glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError(GL_GPU);
    assert(ret);
    assert(ramBuffer);
    if (ret) {
        // update data directly on the mapped buffer
        std::memcpy(ret, (void*)ramBuffer, bytesCount);
        GLboolean result = GL_GPU::glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
        assert(result == GL_TRUE);
        Q_UNUSED(result);
    }
    glCheckError(GL_GPU);

    // copy pixels from PBO to texture object
    // using glBindTexture followed by glTexSubImage2D.
    // Use offset instead of pointer (last parameter is 0).
    tex->fillOrAllocateTexture(textureRectangle, tileRect, true, 0);

    // restore previously bound PBO
    GL_GPU::glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);
    //glBindTexture(GL_TEXTURE_2D, 0); // why should we bind texture 0?
    glCheckError(GL_GPU);

    *texture = tex;

    _imp->updateViewerPboIndex = (_imp->updateViewerPboIndex + 1) % 2;
} // ViewerGL::transferBufferFromRAMtoGPU

void
ViewerGL::clearLastRenderedImage()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ViewerNodePtr internalNode = getInternalNode();

    for (int i = 0; i < 2; ++i) {
        for (U32 j = 0; j < _imp->displayTextures[i].lastRenderedTiles.size(); ++j) {
            _imp->displayTextures[i].lastRenderedTiles[j].reset();
        }
        if (_imp->displayTextures[i].memoryHeldByLastRenderedImages > 0) {
            internalNode->unregisterPluginMemory(_imp->displayTextures[i].memoryHeldByLastRenderedImages);
            _imp->displayTextures[i].memoryHeldByLastRenderedImages = 0;
        }
    }
}

void
ViewerGL::disconnectInputTexture(int textureIndex, bool clearRoD)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
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
    if ( (e)->buttons() == Qt::LeftButton   && (!e->modifiers() &  Qt::MetaModifier) ) {
        _imp->pointerTypeOnPress = ePenTypeLMB;
    } else if ( e->buttons() == Qt::RightButton  || (e->buttons() == Qt::LeftButton  && (e->modifiers() &  Qt::MetaModifier)) ) {
        _imp->pointerTypeOnPress = ePenTypeRMB;
    } else if ( e->buttons() == Qt::MiddleButton  || (e->buttons() == Qt::LeftButton  && (e->modifiers() &  Qt::AltModifier)) ) {
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
    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
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
        overlaysCaught = _imp->viewerTab->notifyOverlaysPenDown( RenderScale(scale), _imp->pointerTypeOnPress, QMouseEventLocalPos(e), zoomPos, _imp->pressureOnPress, currentTimeForEvent(e) );
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
            Q_EMIT selectionCleared();
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
            Q_EMIT selectionRectangleChanged(true);
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
    if ( _imp->viewerTab->notifyOverlaysPenUp(RenderScale(scale), QMouseEventLocalPos(e), zoomPos, currentTimeForEvent(e), _imp->pressureOnRelease) ) {
        mustRedraw = true;
    }
    if (mustRedraw) {
        update();
    }

    if (_imp->renderOnPenUp) {
        _imp->renderOnPenUp = false;
        getInternalNode()->renderCurrentFrame(true);
    }
} // ViewerGL::mouseReleaseEvent

void
ViewerGL::mouseMoveEvent(QMouseEvent* e)
{
    if ( !penMotionInternal(e->x(), e->y(), /*pressure=*/ 1., currentTimeForEvent(e), e) ) {
        QGLWidget::mouseMoveEvent(e);
    }
} // mouseMoveEvent

void
ViewerGL::tabletEvent(QTabletEvent* e)
{
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
        _imp->pressureOnPress = e->pressure();
        QGLWidget::tabletEvent(e);
        break;
    }
    case QEvent::TabletRelease: {
        _imp->pressureOnRelease = e->pressure();
        QGLWidget::tabletEvent(e);
        break;
    }
    case QEvent::TabletMove: {
        if ( !penMotionInternal(e->x(), e->y(), e->pressure(), currentTimeForEvent(e), e) ) {
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
                            double timestamp,
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

    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates(x, y);
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
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
        _imp->viewerTab->getInternalNode()->renderCurrentFrame(true);

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
        Q_EMIT selectionRectangleChanged(false);
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
    
    return true;
} // ViewerGL::penMotionInternal

void
ViewerGL::mouseDoubleClickEvent(QMouseEvent* e)
{
    unsigned int mipMapLevel = _imp->displayTextures[0].mipMapLevel;
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
        if ( _imp->infoViewer[textureIndex]->colorAndMouseVisible() ) {
            _imp->infoViewer[textureIndex]->hideColorAndMouseInfo();
        }
        if (textureIndex == 0) {
            setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
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
    if (!xInitialized || !yInitialized) {
        if ( !viewerNode->isViewersSynchroEnabled() ) {
            pos = mapFromGlobal( QCursor::pos() );
            QMutexLocker l(&_imp->zoomCtxMutex);
            imgPosCanonical = _imp->zoomCtx.toZoomCoordinates( pos.x(), pos.y() );
        } else {
            NodePtr masterViewerNode = getViewerTab()->getGui()->getApp()->getMasterSyncViewer();
            if (masterViewerNode) {
                ViewerNodePtr viewerNode = toViewerNode(masterViewerNode->getEffectInstance());
                ViewerGL* viewerUIContext = dynamic_cast<ViewerGL*>(viewerNode->getUiContext());
                assert(viewerUIContext);
                pos = viewerUIContext->mapFromGlobal( QCursor::pos() );
                imgPosCanonical = viewerUIContext->toZoomCoordinates(pos);
            } else {
                pos = mapFromGlobal( QCursor::pos() );
                QMutexLocker l(&_imp->zoomCtxMutex);
                imgPosCanonical = _imp->zoomCtx.toZoomCoordinates( pos.x(), pos.y() );
            }
        }
    } else {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPosCanonical = _imp->zoomCtx.toZoomCoordinates( pos.x(), pos.y() );
    }

    float r, g, b, a;
    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool picked = false;
    RectD rod = getRoD(textureIndex);
    RectD projectCanonical;
    _imp->getProjectFormatCanonical(projectCanonical);
    unsigned int mmLevel;
    if ( ( imgPosCanonical.x() >= rod.left() ) &&
         ( imgPosCanonical.x() < rod.right() ) &&
         ( imgPosCanonical.y() >= rod.bottom() ) &&
         ( imgPosCanonical.y() < rod.top() ) ) {
        if ( (pos.x() >= 0) && ( pos.x() < width() ) &&
             (pos.y() >= 0) && ( pos.y() < height() ) ) {
            ///if the clip to project format is enabled, make sure it is in the project format too
            bool clipping = viewerNode->isClipToProjectEnabled();
            if ( !clipping ||
                 ( ( imgPosCanonical.x() >= projectCanonical.left() ) &&
                   ( imgPosCanonical.x() < projectCanonical.right() ) &&
                   ( imgPosCanonical.y() >= projectCanonical.bottom() ) &&
                   ( imgPosCanonical.y() < projectCanonical.top() ) ) ) {
                //imgPos must be in canonical coordinates
                picked = getColorAt(imgPosCanonical.x(), imgPosCanonical.y(), linear, textureIndex, &r, &g, &b, &a, &mmLevel);
            }
        }
    }
    if (!picked) {
        _imp->infoViewer[textureIndex]->setColorValid(false);
        if (textureIndex == 0) {
            setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
        }
    } else {
        _imp->infoViewer[textureIndex]->setColorApproximated(mmLevel > 0);
        _imp->infoViewer[textureIndex]->setColorValid(true);
        if ( !_imp->infoViewer[textureIndex]->colorAndMouseVisible() ) {
            _imp->infoViewer[textureIndex]->showColorAndMouseInfo();
        }
        _imp->infoViewer[textureIndex]->setColor(r, g, b, a);

        if (textureIndex == 0) {
            OfxRGBAColourD interactColor = {r,g,b,a};
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
ViewerGL::setParametricParamsPickerColor(const OfxRGBAColourD& color, bool setColor, bool hasColor)
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
        NodeGuiPtr node = nodePanel->getNode();
        if (!node) {
            continue;
        }
        node->getNode()->getEffectInstance()->setInteractColourPicker_public(color, setColor, hasColor);
    }
}

bool
ViewerGL::checkIfViewPortRoIValidOrRenderForInput(int texIndex)
{

    ViewerNodePtr viewerNode = getInternalNode();
    ViewerInstancePtr instance = viewerNode->getInternalViewerNode();
    unsigned int mipMapLevel = (unsigned int)std::max((int)instance->getMipMapLevelFromZoomFactor(), (int)viewerNode->getProxyModeKnobMipMapLevel());
    int closestPo2 = 1 << mipMapLevel;
    if (closestPo2 != _imp->displayTextures[texIndex].texture->getTextureRect().closestPo2) {
        return false;
    }
    RectI roiNotRounded;
    RectI roi = getImageRectangleDisplayedRoundedToTileSize(_imp->displayTextures[texIndex].rod, _imp->displayTextures[texIndex].texture->getTextureRect().par, mipMapLevel, 0, 0, 0, &roiNotRounded);
    const RectI& currentTexRoi = _imp->displayTextures[texIndex].texture->getTextureRect();
    if (!currentTexRoi.contains(roi)) {
        return false;
    }
    _imp->displayTextures[texIndex].roiNotRoundedToTileSize.set(roiNotRounded);

    return true;
}

void
ViewerGL::checkIfViewPortRoIValidOrRender()
{
    for (int i = 0; i < 2; ++i) {
        if ( !checkIfViewPortRoIValidOrRenderForInput(i) ) {
            if ( !getViewerTab()->getGui()->getApp()->getProject()->isLoadingProject() ) {
                ViewerInstancePtr viewer = getInternalNode()->getInternalViewerNode();
                assert(viewer);
                if (viewer) {
                    viewer->getRenderEngine()->abortRenderingAutoRestart();
                    viewer->renderCurrentFrame(true);
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

    if (!_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->getApp()->isDuringPainting()) {
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
    unsigned int oldMipMapLevel, newMipMapLevel;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomFactor = _imp->zoomCtx.factor();

        oldMipMapLevel = std::log( zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) ) ) / M_LN2;

        zoomFactor *= scaleFactor;

        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }

        newMipMapLevel = std::log( zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) ) ) / M_LN2;
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


    ///Clear green cached line so the user doesn't expect to see things in the cache
    ///since we're changing the zoom factor
    if (oldMipMapLevel != newMipMapLevel) {
        _imp->viewerTab->clearTimelineCacheLine();
    }
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
    unsigned int oldMipMapLevel, newMipMapLevel;
    newMipMapLevel = std::log( newZoomFactor >= 1 ? 1 :
                               std::pow( 2, -std::ceil(std::log(newZoomFactor) / M_LN2) ) ) / M_LN2;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        oldMipMapLevel = std::log( _imp->zoomCtx.factor() >= 1 ? 1 :
                                   std::pow( 2, -std::ceil(std::log( _imp->zoomCtx.factor() ) / M_LN2) ) ) / M_LN2;
        double scale = newZoomFactor / _imp->zoomCtx.factor();
        double centerX = ( _imp->zoomCtx.left() + _imp->zoomCtx.right() ) / 2.;
        double centerY = ( _imp->zoomCtx.top() + _imp->zoomCtx.bottom() ) / 2.;
        _imp->zoomCtx.zoom(centerX, centerY, scale);
        _imp->zoomOrPannedSinceLastFit = true;
    }
    ///Clear green cached line so the user doesn't expect to see things in the cache
    ///since we're changing the zoom factor
    if (newMipMapLevel != oldMipMapLevel) {
        _imp->viewerTab->clearTimelineCacheLine();
    }

    checkIfViewPortRoIValidOrRender();
}

void
ViewerGL::fitImageToFormat()
{
    if (_imp->viewerTab && _imp->viewerTab->getGui() && !_imp->viewerTab->getGui()->getApp()->isDuringPainting()) {
        fitImageToFormat(false);
    }
}

void
ViewerGL::fitImageToFormat(bool useProjectFormat)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    // size in Canonical = Zoom coordinates !
    double w, h;
    const RectD& tex0RoD = _imp->displayTextures[0].rod;
    if (!tex0RoD.isNull() && !tex0RoD.isInfinite() && !useProjectFormat) {
        w = tex0RoD.width();
        h = tex0RoD.height();
    } else {
        h = _imp->projectFormat.height();
        w = _imp->projectFormat.width() * _imp->projectFormat.getPixelAspectRatio();
    }

    assert(h > 0. && w > 0.);

    double old_zoomFactor;
    double zoomFactor;
    unsigned int oldMipMapLevel, newMipMapLevel;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        old_zoomFactor = _imp->zoomCtx.factor();
        oldMipMapLevel = std::log( old_zoomFactor >= 1 ? 1 :
                                   std::pow( 2, -std::ceil(std::log(old_zoomFactor) / M_LN2) ) ) / M_LN2;

        // set the PAR first
        //_imp->zoomCtx.setZoom(0., 0., 1., 1.);
        // leave 4% of margin around
        _imp->zoomCtx.fit(-0.02 * w, 1.02 * w, -0.02 * h, 1.02 * h);
        zoomFactor = _imp->zoomCtx.factor();
        _imp->zoomOrPannedSinceLastFit = false;
    }
    newMipMapLevel = std::log( zoomFactor >= 1 ? 1 :
                               std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) ) ) / M_LN2;

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

    if (newMipMapLevel != oldMipMapLevel) {
        ///Clear green cached line so the user doesn't expect to see things in the cache
        ///since we're changing the zoom factor
        _imp->viewerTab->clearTimelineCacheLine();
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
        {
            QMutexLocker k(&_imp->projectFormatMutex);
            f = _imp->projectFormat;
        }
        RectD canonicalFormat = f.toCanonicalFormat();
        for (int i = 0; i < 2; ++i) {
            setRegionOfDefinition(canonicalFormat, f.getPixelAspectRatio(), i);
        }
    }
    //resetWipeControls();
    clearViewer();
}

/* The dataWindow of the currentFrame(BBOX) in canonical coordinates */
const RectD&
ViewerGL::getRoD(int textureIndex) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->displayTextures[textureIndex].rod;
}

/*The displayWindow of the currentFrame(Resolution)
   This is the same for both as we only use the display window as to indicate the project window.*/
Format
ViewerGL::getDisplayWindow() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->displayTextures[0].format;
}

void
ViewerGL::setRegionOfDefinition(const RectD & rod,
                                double par,
                                int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( !_imp->viewerTab->getGui() ) {
        return;
    }

    RectI pixelRoD;
    rod.toPixelEnclosing(0, par, &pixelRoD);

    _imp->displayTextures[textureIndex].rod = rod;
    if ( _imp->infoViewer[textureIndex] && !_imp->viewerTab->getGui()->isGUIFrozen() ) {
        _imp->infoViewer[textureIndex]->setDataWindow(pixelRoD);
    }


    QString left, btm, right, top;
    left.setNum( pixelRoD.left() );
    btm.setNum( pixelRoD.bottom() );
    right.setNum( pixelRoD.right() );
    top.setNum( pixelRoD.top() );


    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].append(left);
    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].append( QLatin1Char(',') );
    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].append(btm);
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].append(right);
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].append( QLatin1Char(',') );
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].append(top);
}

void
ViewerGL::onProjectFormatChangedInternal(const Format & format,
                                         bool triggerRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !_imp->viewerTab->getGui() ) {
        return;
    }
    RectD canonicalFormat = format.toCanonicalFormat();

    for (int i = 0; i < 2; ++i) {
        if (_imp->infoViewer[i]) {
            _imp->infoViewer[i]->setResolution(format);
            if (!_imp->displayTextures[i].isVisible) {
                _imp->displayTextures[i].rod = canonicalFormat;
            }
        }
    }
    {
        QMutexLocker k(&_imp->projectFormatMutex);
        _imp->projectFormat = format;
    }
    _imp->currentViewerInfo_resolutionOverlay.clear();
    _imp->currentViewerInfo_resolutionOverlay.append( QString::number( format.width() ) );
    _imp->currentViewerInfo_resolutionOverlay.append( QLatin1Char('x') );
    _imp->currentViewerInfo_resolutionOverlay.append( QString::number( format.height() ) );

    bool loadingProject = _imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject();
    if (!loadingProject && triggerRender) {
        fitImageToFormat(true);
    }

    if (!loadingProject) {
        update();
    }
} // ViewerGL::onProjectFormatChangedInternal

void
ViewerGL::onProjectFormatChanged(const Format & format)
{
    onProjectFormatChangedInternal(format, true);
}


/*display black in the viewer*/
void
ViewerGL::clearViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->displayTextures[0].isVisible = false;
    _imp->displayTextures[1].isVisible = false;
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
ViewerGL::leaveEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (_imp->pickerState == ePickerStateInactive) {
        _imp->infoViewer[0]->hideColorAndMouseInfo();
        _imp->infoViewer[1]->hideColorAndMouseInfo();
        setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
    }
    QGLWidget::leaveEvent(e);
}

void
ViewerGL::resizeEvent(QResizeEvent* e)
{ // public to hack the protected field
  // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QGLWidget::resizeEvent(e);
}

ImageBitDepthEnum
ViewerGL::getBitDepth() const
{
    return appPTR->getCurrentSettings()->getViewersBitDepth();
}

bool
ViewerGL::renderText(double x,
                     double y,
                     const std::string &string,
                     double r,
                     double g,
                     double b)
{
    QColor c;

    c.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    renderText( x, y, QString::fromUtf8( string.c_str() ), c, font() );

    return true;
}

void
ViewerGL::renderText(double x,
                     double y,
                     const QString &text,
                     const QColor &color,
                     const QFont &font)
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
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font);
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

        NodePtr node = isNodePanel->getNode()->getNode();
        if (!node) {
            continue;
        }

        QString mess;
        int nType;
        node->getPersistentMessage(&mess, &nType);
        if ( !mess.isEmpty() ) {
            allMessages.append(mess);
            ++nbNonEmpty;
        }
        if ( next != openedPanels.end() ) {
            ++next;
        }

        if ( !mess.isEmpty() ) {
            type = (nbNonEmpty == 1 && nType == 2) ? 2 : 1;
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
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    update();
}

void
ViewerGL::redrawNow()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    updateGL();
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
ViewerGL::makeOpenGLcontextCurrent()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();
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
                            bool /*pickInput*/)
{

#pragma message WARN("Todo: use pickInput")
    float r, g, b, a;
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
        unsigned int mmLevel;
        bool picked = getColorAt(imgPos.x(), imgPos.y(), linear, i, &r, &g, &b, &a, &mmLevel);
        if (picked) {
            if (i == 0) {
                _imp->viewerTab->getGui()->setColorPickersColor(r, g, b, a);
            }
            _imp->infoViewer[i]->setColorApproximated(mmLevel > 0);
            _imp->infoViewer[i]->setColorValid(true);
            if ( !_imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->showColorAndMouseInfo();
            }
            _imp->infoViewer[i]->setColor(r, g, b, a);
            ret = true;
            {
                OfxRGBAColourD interactColor = {r,g,b,a};
                setParametricParamsPickerColor(interactColor, true, true);
            }
        } else {
            _imp->infoViewer[i]->setColorValid(false);
            setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
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
    Format dispW = getDisplayWindow();
    RectD canonicalDispW = dispW.toCanonicalFormat();
    bool isDrawing = _imp->viewerTab->getGui()->getApp()->isDuringPainting();

    if (!isDrawing) {
        for (int i = 0; i < 2; ++i) {
            const RectD& rod = getRoD(i);
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
        if ( viewerNode->isClipToProjectEnabled() &&
             ( ( imgPos.x() < dispW.left() ) ||
               ( imgPos.x() >= dispW.right() ) ||
               ( imgPos.y() < dispW.bottom() ) ||
               ( imgPos.y() >= dispW.top() ) ) ) {
                 if ( _imp->infoViewer[texIndex]->colorAndMouseVisible() && _imp->pickerState == ePickerStateInactive) {
                     _imp->infoViewer[texIndex]->hideColorAndMouseInfo();
                 }
                 for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
                     if ( (*it)->getViewerTextureInputDisplayed() == texIndex ) {
                         (*it)->hideViewerCursor();
                     }
                 }
                 if (texIndex == 0 && _imp->pickerState == ePickerStateInactive) {
                     setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
                 }
             } else {
                 if (_imp->pickerState == ePickerStateInactive) {
                     //if ( !_imp->viewerTab->getInternalNode()->getRenderEngine()->hasThreadsWorking() ) {
                     updateColorPicker( texIndex, widgetPos.x(), widgetPos.y() );
                // }
            } else if ( ( _imp->pickerState == ePickerStatePoint) || ( _imp->pickerState == ePickerStateRectangle) ) {
                if ( !_imp->infoViewer[texIndex]->colorAndMouseVisible() ) {
                    _imp->infoViewer[texIndex]->showColorAndMouseInfo();
                }
                // Show the picker on parametric params without updating the color value
                if (texIndex == 0) {
                    setParametricParamsPickerColor(OfxRGBAColourD(), false, true);
                }
            } else {
                ///unkwn state
                assert(false);
            }
            double par = _imp->displayTextures[texIndex].format.getPixelAspectRatio();
            QPoint imgPosPixel;
            imgPosPixel.rx() = std::floor(imgPos.x() / par);
            imgPosPixel.ry() = std::floor( imgPos.y() );
            _imp->infoViewer[texIndex]->setMousePos(imgPosPixel);
        }
    } else {
        if ( _imp->infoViewer[texIndex]->colorAndMouseVisible() && _imp->pickerState == ePickerStateInactive) {
            _imp->infoViewer[texIndex]->hideColorAndMouseInfo();
        }
        for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
            if ( (*it)->getViewerTextureInputDisplayed() == texIndex ) {
                (*it)->hideViewerCursor();
            }
        }
        if (texIndex == 0 && _imp->pickerState == ePickerStateInactive) {
            setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
        }
    }
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

    rect.set_left( std::min( topLeft.x(), btmRight.x() ) );
    rect.set_right( std::max( topLeft.x(), btmRight.x() ) );
    rect.set_bottom( std::min( topLeft.y(), btmRight.y() ) );
    rect.set_top( std::max( topLeft.y(), btmRight.y() ) );
    for (int i = 0; i < 2; ++i) {
        unsigned int mm;
        bool picked = getColorAtRect(rect, linear, i, &r, &g, &b, &a, &mm);
        if (picked) {
            if (i == 0) {
                _imp->viewerTab->getGui()->setColorPickersColor(r, g, b, a);
            }
            _imp->infoViewer[i]->setColorValid(true);
            if ( !_imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->showColorAndMouseInfo();
            }
            _imp->infoViewer[i]->setColorApproximated(mm > 0);
            _imp->infoViewer[i]->setColor(r, g, b, a);

            OfxRGBAColourD c = {r,g,b,a};
            setParametricParamsPickerColor(c, true, true);
        } else {
            _imp->infoViewer[i]->setColorValid(false);
            setParametricParamsPickerColor(OfxRGBAColourD(), false, false);
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
    makeCurrent();

    *r = 0;
    *g = 0;
    *b = 0;
    *a = 0;

    Texture::DataTypeEnum type;
    if (_imp->displayTextures[0].texture) {
        type = _imp->displayTextures[0].texture->type();
    } else if (_imp->displayTextures[1].texture) {
        type = _imp->displayTextures[1].texture->type();
    } else {
        return;
    }

    QPointF pos;
    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        pos = _imp->zoomCtx.toWidgetCoordinates(x, y);
    }

    if (type == Texture::eDataTypeByte) {
        U32 pixel;
        GL_GPU::glReadBuffer(GL_FRONT);
        GL_GPU::glReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixel);
        U8 red = 0, green = 0, blue = 0, alpha = 0;
        blue |= pixel;
        green |= (pixel >> 8);
        red |= (pixel >> 16);
        alpha |= (pixel >> 24);
        *r = (double)red / 255.;
        *g = (double)green / 255.;
        *b = (double)blue / 255.;
        *a = (double)alpha / 255.;
        glCheckError(GL_GPU);
    } else if (type == Texture::eDataTypeFloat) {
        GLfloat pixel[4];
        GL_GPU::glReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_RGBA, GL_FLOAT, pixel);
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

    GL_GPU::glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack(GL_GPU);
    GL_GPU::glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack(GL_GPU);
    GL_GPU::glPushClientAttrib(GL_ALL_ATTRIB_BITS);
    GL_GPU::glMatrixMode(GL_PROJECTION);
    glCheckProjectionStack(GL_GPU);
    GL_GPU::glPushMatrix();
    GL_GPU::glMatrixMode(GL_MODELVIEW);
    glCheckModelviewStack(GL_GPU);
    GL_GPU::glPushMatrix();

    // set defaults to work around OFX plugin bugs
    GL_GPU::glEnable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

// used by the RAII class OGLContextSaver
void
ViewerGL::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::glBindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    GL_GPU::glMatrixMode(GL_PROJECTION);
    GL_GPU::glPopMatrix();
    GL_GPU::glMatrixMode(GL_MODELVIEW);
    GL_GPU::glPopMatrix();
    GL_GPU::glPopClientAttrib();
    GL_GPU::glPopAttrib();
}

void
ViewerGL::clearLastRenderedTexture()
{
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        U64 toUnRegister = 0;
        for (int i = 0; i < 2; ++i) {
            for (U32 j = 0; j < _imp->displayTextures[i].lastRenderedTiles.size(); ++j) {
                _imp->displayTextures[i].lastRenderedTiles[j].reset();
            }
            toUnRegister += _imp->displayTextures[i].memoryHeldByLastRenderedImages;
        }
        if (toUnRegister > 0) {
            getInternalNode()->unregisterPluginMemory(toUnRegister);
        }
    }
}

ImagePtr
ViewerGL::getLastRenderedImage(int textureIndex) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !getInternalNode()->getNode()->isActivated() ) {
        return ImagePtr();
    }
    QMutexLocker l(&_imp->lastRenderedImageMutex);
    for (U32 i = 0; i < _imp->displayTextures[textureIndex].lastRenderedTiles.size(); ++i) {
        ImagePtr mipmap = _imp->displayTextures[textureIndex].lastRenderedTiles[i];
        if (mipmap) {
            return mipmap;
        }
    }

    return ImagePtr();
}

ImagePtr
ViewerGL::getLastRenderedImageByMipMapLevel(int textureIndex,
                                            unsigned int mipMapLevel) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !getInternalNode()->getNode()->isActivated() ) {
        return ImagePtr();
    }

    QMutexLocker l(&_imp->lastRenderedImageMutex);
    assert(_imp->displayTextures[textureIndex].lastRenderedTiles.size() > mipMapLevel);

    ImagePtr mipmap = _imp->displayTextures[textureIndex].lastRenderedTiles[mipMapLevel];
    if (mipmap) {
        return mipmap;
    }

    //Find an image at higher scale
    if (mipMapLevel > 0) {
        for (int i = (int)mipMapLevel - 1; i >= 0; --i) {
            mipmap = _imp->displayTextures[textureIndex].lastRenderedTiles[i];
            if (mipmap) {
                return mipmap;
            }
        }
    }

    //Find an image at lower scale
    for (U32 i = mipMapLevel + 1; i < _imp->displayTextures[textureIndex].lastRenderedTiles.size(); ++i) {
        mipmap = _imp->displayTextures[textureIndex].lastRenderedTiles[i];
        if (mipmap) {
            return mipmap;
        }
    }

    return ImagePtr();
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
    int mmLvl = _imp->displayTextures[0].mipMapLevel;
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
getColorAtInternal(const ImagePtr& image,
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
    if ( image->getBounds().contains(x, y) ) {
        Image::ReadAccess racc( image.get() );
        const PIX* pix = (const PIX*)racc.pixelAt(x, y);

        if (!pix) {
            return false;
        }

        int nComps = image->getComponents().getNumComponents();
        if (nComps >= 4) {
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = pix[2] / (float)maxValue;
            *a = pix[3] / (float)maxValue;
        } else if (nComps == 3) {
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = pix[2] / (float)maxValue;
            *a = 1.;
        } else if (nComps == 2) {
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = 1.;
            *a = 1.;
        } else {
            *r = *g = *b = *a = pix[0] / (float)maxValue;
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
    }


    return false;
} // getColorAtInternal

bool
ViewerGL::getColorAt(double x,
                     double y,           // x and y in canonical coordinates
                     bool forceLinear,
                     int textureIndex,
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


    unsigned int mipMapLevel = (unsigned int)getMipMapLevelCombinedToZoomFactor();
    ImagePtr image = getLastRenderedImageByMipMapLevel(textureIndex, mipMapLevel);


    if (!image) {
        return false;
        ///Don't do this as this is 8bit data
        /*double colorGPU[4];
           getTextureColorAt(x, y, &colorGPU[0], &colorGPU[1], &colorGPU[2], &colorGPU[3]);
         * a = colorGPU[3];
           if ( forceLinear && (_imp->displayingImageLut != eViewerColorSpaceLinear) ) {
            const Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);

         * r = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[0]);
         * g = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[1]);
         * b = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[2]);
           } else {
         * r = colorGPU[0];
         * g = colorGPU[1];
         * b = colorGPU[2];
           }
           return true;*/
    }

    ImageBitDepthEnum depth = image->getBitDepth();
    ViewerColorSpaceEnum srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Color::Lut* dstColorSpace;
    const Color::Lut* srcColorSpace;
    if ( (srcCS == _imp->displayingImageLut)
         && ( (_imp->displayingImageLut == eViewerColorSpaceLinear) || !forceLinear ) ) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        if ( image->getComponents().isColorPlane() ) {
            srcColorSpace = ViewerInstance::lutFromColorspace(srcCS);
            dstColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
        } else {
            srcColorSpace = dstColorSpace = 0;
        }
    }

    const double par = image->getPixelAspectRatio();
    double scale = 1. / ( 1 << image->getMipMapLevel() );

    ///Convert to pixel coords
    int xPixel = std::floor(x  * scale / par);
    int yPixel = std::floor(y * scale);
    bool gotval;
    switch (depth) {
    case eImageBitDepthByte:
        gotval = getColorAtInternal<unsigned char, 255>(image,
                                                        xPixel, yPixel,
                                                        forceLinear,
                                                        srcColorSpace,
                                                        dstColorSpace,
                                                        r, g, b, a);
        break;
    case eImageBitDepthShort:
        gotval = getColorAtInternal<unsigned short, 65535>(image,
                                                           xPixel, yPixel,
                                                           forceLinear,
                                                           srcColorSpace,
                                                           dstColorSpace,
                                                           r, g, b, a);
        break;
    case eImageBitDepthFloat:
        gotval = getColorAtInternal<float, 1>(image,
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
    *imgMmlevel = image->getMipMapLevel();

    return gotval;
} // getColorAt

bool
ViewerGL::getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                         bool forceLinear,
                         int textureIndex,
                         float* r,
                         float* g,
                         float* b,
                         float* a,
                         unsigned int* imgMm)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);

    unsigned int mipMapLevel = (unsigned int)getMipMapLevelCombinedToZoomFactor();
    ImagePtr image = getLastRenderedImageByMipMapLevel(textureIndex, mipMapLevel);

    if (image) {
        mipMapLevel = image->getMipMapLevel();
        *imgMm = mipMapLevel;
    }

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
    if (!image) {
        return false;
        //don't do this as this is 8 bit
        /*
           Texture::DataTypeEnum type;
           if (_imp->displayTextures[0]) {
            type = _imp->displayTextures[0]->type();
           } else if (_imp->displayTextures[1]) {
            type = _imp->displayTextures[1]->type();
           } else {
            return false;
           }

           if ( (type == Texture::eDataTypeByte) ) {
            std::vector<U32> pixels(rectPixel.width() * rectPixel.height());
            GL_GPU::glReadBuffer(GL_FRONT);
            GL_GPU::glReadPixels(rectPixel.left(), rectPixel.right(), rectPixel.width(), rectPixel.height(),
                         GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &pixels.front());
            double rF,gF,bF,aF;
            for (U32 i = 0 ; i < pixels.size(); ++i) {
                U8 red = 0, green = 0, blue = 0, alpha = 0;
                blue |= pixels[i];
                green |= (pixels[i] >> 8);
                red |= (pixels[i] >> 16);
                alpha |= (pixels[i] >> 24);
                rF = (double)red / 255.;
                gF = (double)green / 255.;
                bF = (double)blue / 255.;
                aF = (double)alpha / 255.;

                aSum += aF;
                if ( forceLinear && (_imp->displayingImageLut != eViewerColorSpaceLinear) ) {
                    const Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);

                    rSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(rF);
                    gSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(gF);
                    bSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(bF);
                } else {
                    rSum += rF;
                    gSum += gF;
                    bSum += bF;
                }

            }

            glCheckError(GL_GPU);
           } else if ( (type == Texture::eDataTypeFloat)) {
            std::vector<float> pixels(rectPixel.width() * rectPixel.height() * 4);
            GL_GPU::glReadPixels(rectPixel.left(), rectPixel.right(), rectPixel.width(), rectPixel.height(),
                         GL_RGBA, GL_FLOAT, &pixels.front());

            int rowSize = rectPixel.width() * 4;
            for (int y = 0; y < rectPixel.height(); ++y) {
                for (int x = 0; x < rectPixel.width(); ++x) {
                    double rF = pixels[y * rowSize + (4 * x)];
                    double gF = pixels[y * rowSize + (4 * x) + 1];
                    double bF = pixels[y * rowSize + (4 * x) + 2];
                    double aF = pixels[y * rowSize + (4 * x) + 3];

                    aSum += aF;
                    if ( forceLinear && (_imp->displayingImageLut != eViewerColorSpaceLinear) ) {
                        const Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);

                        rSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(rF);
                        gSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(gF);
                        bSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(bF);
                    } else {
                        rSum += rF;
                        gSum += gF;
                        bSum += bF;
                    }
                }
            }


            glCheckError(GL_GPU);
           }

         * r = rSum / rectPixel.area();
         * g = gSum / rectPixel.area();
         * b = bSum / rectPixel.area();
         * a = aSum / rectPixel.area();

           return true;*/
    }


    ImageBitDepthEnum depth = image->getBitDepth();
    ViewerColorSpaceEnum srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(depth);
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

    unsigned long area = 0;
    for (int yPixel = rectPixel.bottom(); yPixel < rectPixel.top(); ++yPixel) {
        for (int xPixel = rectPixel.left(); xPixel < rectPixel.right(); ++xPixel) {
            float rPix, gPix, bPix, aPix;
            bool gotval = false;
            switch (depth) {
            case eImageBitDepthByte:
                gotval = getColorAtInternal<unsigned char, 255>(image,
                                                                xPixel, yPixel,
                                                                forceLinear,
                                                                srcColorSpace,
                                                                dstColorSpace,
                                                                &rPix, &gPix, &bPix, &aPix);
                break;
            case eImageBitDepthShort:
                gotval = getColorAtInternal<unsigned short, 65535>(image,
                                                                   xPixel, yPixel,
                                                                   forceLinear,
                                                                   srcColorSpace,
                                                                   dstColorSpace,
                                                                   &rPix, &gPix, &bPix, &aPix);
                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                gotval = getColorAtInternal<float, 1>(image,
                                                      xPixel, yPixel,
                                                      forceLinear,
                                                      srcColorSpace,
                                                      dstColorSpace,
                                                      &rPix, &gPix, &bPix, &aPix);
                break;
            case eImageBitDepthNone:
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

int
ViewerGL::getCurrentlyDisplayedTime() const
{
    QMutexLocker k(&_imp->lastRenderedImageMutex);

    if (_imp->displayTextures[0].isVisible) {
        return _imp->displayTextures[0].time;
    } else {
        return _imp->viewerTab->getTimeLine()->currentFrame();
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
ViewerGL::setTimelineBounds(double first, double last)
{
    getViewerTab()->setTimelineBounds(first, last);
}

void
ViewerGL::setTripleSyncEnabled(bool toggled)
{
    getViewerTab()->setTripleSyncEnabled(toggled);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ViewerGL.cpp"

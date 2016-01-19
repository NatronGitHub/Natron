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
#include <stdexcept>

#include "Global/Macros.h"
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QApplication> // qApp
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
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

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
#include "Gui/NodeSettingsPanel.h"
#include "Gui/Shaders.h"
#include "Gui/Texture.h"
#include "Gui/ViewerTab.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

#define USER_ROI_BORDER_TICK_SIZE 15.f
#define USER_ROI_CROSS_RADIUS 15.f
#define USER_ROI_SELECTION_POINT_SIZE 8.f
#define USER_ROI_CLICK_TOLERANCE 8.f

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

NATRON_NAMESPACE_USING
using std::cout; using std::endl;



ViewerGL::ViewerGL(ViewerTab* parent,
                   const QGLWidget* shareWidget)
    : QGLWidget(parent,shareWidget)
      , _imp( new Implementation(this, parent) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    QObject::connect( parent->getGui()->getApp()->getProject().get(),SIGNAL( formatChanged(Format) ),this,SLOT( onProjectFormatChanged(Format) ) );

    Format projectFormat;
    parent->getGui()->getApp()->getProject()->getProjectDefaultFormat(&projectFormat);

    RectD canonicalFormat = projectFormat.toCanonicalFormat();
    
    _imp->blankViewerInfo.setRoD(canonicalFormat);
    _imp->blankViewerInfo.setDisplayWindow(projectFormat);
    setRegionOfDefinition(_imp->blankViewerInfo.getRoD(),_imp->blankViewerInfo.getDisplayWindow().getPixelAspectRatio(),0);
    setRegionOfDefinition(_imp->blankViewerInfo.getRoD(),_imp->blankViewerInfo.getDisplayWindow().getPixelAspectRatio(),1);
    onProjectFormatChangedInternal(projectFormat,false);
    resetWipeControls();
    populateMenu();

    QObject::connect( appPTR, SIGNAL(checkerboardSettingsChanged()), this, SLOT(onCheckerboardSettingsChanged()));
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
ViewerGL::resizeGL(int w,
                   int h)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( (h == 0) || (w == 0) ) { // prevent division by 0
        return;
    }
    glCheckError();
    assert(w == width() && h == height()); // if this crashes here, then the viewport size has to be stored to compute glShadow
    glViewport (0, 0, w, h);
    bool zoomSinceLastFit;
    int oldWidth,oldHeight;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        oldWidth = _imp->zoomCtx.screenWidth();
        oldHeight = _imp->zoomCtx.screenHeight();
        _imp->zoomCtx.setScreenSize(w, h);
        zoomSinceLastFit = _imp->zoomOrPannedSinceLastFit;
    }
    glCheckError();
    _imp->ms = eMouseStateUndefined;
    assert(_imp->viewerTab);
    ViewerInstance* viewer = _imp->viewerTab->getInternalNode();
    assert(viewer);
    if (!zoomSinceLastFit) {
        fitImageToFormat();
    }
    if ( viewer->getUiContext() && _imp->viewerTab->getGui() &&
         !_imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject() &&
         ( ( oldWidth != w) || ( oldHeight != h) ) ) {
        viewer->renderCurrentFrame(false);
        
        if (!_imp->persistentMessages.empty()) {
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
            glEnable(GL_BLEND);
        }
        switch (premult) {
            case Natron::eImagePremultiplicationPremultiplied:
                glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
                break;
            case Natron::eImagePremultiplicationUnPremultiplied:
                glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
                break;
            case Natron::eImagePremultiplicationOpaque:
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
        glCheckError();

        return;
    }

    {
        //GLProtectAttrib a(GL_TRANSFORM_BIT); // GL_MODELVIEW is active by default

        // Note: the OFX spec says that the GL_MODELVIEW should be the identity matrix
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectOverlays
        // However,
        // - Nuke uses a different matrix (RoD.width is the width of the RoD of the displayed image)
        // - Nuke transforms the interacts using the modelview if there are Transform nodes between the viewer and the interact.


        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, -1, 1);
        _imp->glShadow.setX( (zoomRight - zoomLeft)/width() );
        _imp->glShadow.setY( (zoomTop - zoomBottom)/height() );
        //glScalef(RoD.width, RoD.width, 1.0); // for compatibility with Nuke
        //glTranslatef(1, 1, 0);     // for compatibility with Nuke
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        //glTranslatef(-1, -1, 0);        // for compatibility with Nuke
        //glScalef(1/RoD.width, 1./RoD.width, 1.0); // for compatibility with Nuke

        glCheckError();


        // don't even bind the shader on 8-bits gamma-compressed textures
        ViewerCompositingOperatorEnum compOp = _imp->viewerTab->getCompositingOperator();

        ///Determine whether we need to draw each texture or not
        int activeInputs[2];
        ViewerInstance* internalViewer = _imp->viewerTab->getInternalNode();
        if (!internalViewer) {
            return;
        }
        internalViewer->getActiveInputs(activeInputs[0], activeInputs[1]);
        bool drawTexture[2];
        drawTexture[0] = _imp->activeTextures[0];
        drawTexture[1] = _imp->activeTextures[1] && compOp != eViewerCompositingOperatorNone;
        if ( (activeInputs[0] == activeInputs[1]) && (compOp != eViewerCompositingOperatorMinus) ) {
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
            GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

            clearColorBuffer( _imp->clearColor.redF(),_imp->clearColor.greenF(),_imp->clearColor.blueF(),_imp->clearColor.alphaF() );
            glCheckErrorIgnoreOSXBug();

            glEnable (GL_TEXTURE_2D);
            glColor4d(1., 1., 1., 1.);
            glBlendColor(1, 1, 1, wipeMix);
            
            ///Depending on the premultiplication of the input image we use a different blending func
            ImagePremultiplicationEnum premultA = _imp->displayingImagePremult[0];
            if (!_imp->viewerTab->isCheckerboardEnabled()) {
                premultA = Natron::eImagePremultiplicationOpaque; ///When no checkerboard, draw opaque
            }

            if (compOp == eViewerCompositingOperatorWipe) {
                ///In wipe mode draw first the input A then only the portion we are interested in the input B

                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, _imp->activeTextures[1] ? eDrawPolygonModeWhole : eDrawPolygonModeWipeLeft);
                }
                if (drawTexture[1]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[1], 1, eDrawPolygonModeWipeRight);
                    glDisable(GL_BLEND);
                }
            } else if (compOp == eViewerCompositingOperatorMinus) {
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, _imp->activeTextures[1] ? eDrawPolygonModeWhole : eDrawPolygonModeWipeLeft);
                }
                if (drawTexture[1]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
                    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[1], 1, eDrawPolygonModeWipeRight);
                    glDisable(GL_BLEND);
                }
            } else if (compOp == eViewerCompositingOperatorUnder) {
                if (drawTexture[0]) {
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, _imp->activeTextures[1] ? eDrawPolygonModeWhole : eDrawPolygonModeWipeLeft);
                }
                if (drawTexture[1]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[1], 1, eDrawPolygonModeWipeRight);
                    glDisable(GL_BLEND);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[1], 1, eDrawPolygonModeWipeRight);
                    glDisable(GL_BLEND);
                }
            } else if (compOp == eViewerCompositingOperatorOver) {
                ///draw first B then A
                if (drawTexture[1]) {
                    ///Depending on the premultiplication of the input image we use a different blending func
                    ImagePremultiplicationEnum premultB = _imp->displayingImagePremult[1];
                    if (!_imp->viewerTab->isCheckerboardEnabled()) {
                        premultB = Natron::eImagePremultiplicationOpaque; ///When no checkerboard, draw opaque
                    }
                    BlendSetter b(premultB);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[1], 1, eDrawPolygonModeWipeRight);
                }
                if (drawTexture[0]) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, eDrawPolygonModeWipeRight);
                    glDisable(GL_BLEND);

                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, eDrawPolygonModeWipeLeft);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, eDrawPolygonModeWipeRight);
                    glDisable(GL_BLEND);
                }
            } else {
                if (drawTexture[0]) {
                    glDisable(GL_BLEND);
                    BlendSetter b(premultA);
                    _imp->drawRenderingVAO(_imp->displayingImageMipMapLevel[0], 0, eDrawPolygonModeWhole);
                }
            }
        } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

        ///Unbind render textures for overlays
        glBindTexture(GL_TEXTURE_2D, savedTexture);
        
        glCheckError();
        if (_imp->overlay) {
            drawOverlay(getCurrentRenderScale());
        } else {
            const QFont& f = font();
            QFontMetrics fm(f);
            QPointF pos;
            {
                QMutexLocker k(&_imp->zoomCtxMutex);
                pos = _imp->zoomCtx.toZoomCoordinates(10, height() - fm.height());
            }
            renderText(pos.x(), pos.y(), tr("Overlays off"), QColor(200,0,0), f);
        }
        
        if (_imp->ms == eMouseStateSelecting) {
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
    glCheckError();
    {
        GLProtectAttrib att(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

        glClearColor(r,g,b,a);
        glClear(GL_COLOR_BUFFER_BIT);
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glCheckErrorIgnoreOSXBug();
}

void
ViewerGL::toggleOverlays()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->overlay = !_imp->overlay;
    update();
}

void
ViewerGL::toggleWipe()
{
    if (getViewerTab()->getCompositingOperator() != Natron::eViewerCompositingOperatorNone) {
        getViewerTab()->setCompositingOperator(Natron::eViewerCompositingOperatorNone);
    } else {
        getViewerTab()->setCompositingOperator(Natron::eViewerCompositingOperatorWipe);
    }
}

void
ViewerGL::centerWipe()
{
    
    QPoint pos = mapFromGlobal(QCursor::pos());
    QPointF zoomPos = toZoomCoordinates(QPointF(pos.x(), pos.y()));
    
    {
        QMutexLocker l(&_imp->wipeControlsMutex);
        _imp->wipeCenter.rx() = zoomPos.x();
        _imp->wipeCenter.ry() = zoomPos.y();
    }
    update();
}

void
ViewerGL::drawOverlay(unsigned int mipMapLevel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    glCheckError();
    
    RectD projectFormatCanonical;
    _imp->getProjectFormatCanonical(projectFormatCanonical);
    renderText(projectFormatCanonical.right(),projectFormatCanonical.bottom(), _imp->currentViewerInfo_resolutionOverlay,_imp->textRenderingColor,*_imp->textFont);


    QPoint topRight( projectFormatCanonical.right(),projectFormatCanonical.top() );
    QPoint topLeft( projectFormatCanonical.left(),projectFormatCanonical.top() );
    QPoint btmLeft( projectFormatCanonical.left(),projectFormatCanonical.bottom() );
    QPoint btmRight( projectFormatCanonical.right(),projectFormatCanonical.bottom() );

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

        int activeInputs[2];
        getInternalNode()->getActiveInputs(activeInputs[0], activeInputs[1]);
        for (int i = 0; i < 2; ++i) {
            
            if (!_imp->activeTextures[i] || activeInputs[i] == -1) {
                continue;
            }
            if (i == 1 && (_imp->viewerTab->getCompositingOperator() == eViewerCompositingOperatorNone)) {
                break;
            }
            RectD dataW = getRoD(i);
            
            if (dataW != projectFormatCanonical) {
                renderText(dataW.right(), dataW.top(),
                           _imp->currentViewerInfo_topRightBBOXoverlay[i], _imp->rodOverlayColor,*_imp->textFont);
                renderText(dataW.left(), dataW.bottom(),
                           _imp->currentViewerInfo_btmLeftBBOXoverlay[i], _imp->rodOverlayColor,*_imp->textFont);
                glCheckError();

                QPointF topRight2( dataW.right(), dataW.top() );
                QPointF topLeft2( dataW.left(),dataW.top() );
                QPointF btmLeft2( dataW.left(),dataW.bottom() );
                QPointF btmRight2( dataW.right(),dataW.bottom() );
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
                glCheckErrorIgnoreOSXBug();
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

        ViewerCompositingOperatorEnum compOperator = _imp->viewerTab->getCompositingOperator();
        if (compOperator != eViewerCompositingOperatorNone) {
            drawWipeControl();
        }


        glCheckError();
        glColor4f(1., 1., 1., 1.);
        double scale = 1. / (1 << mipMapLevel);
        
        /*
         Draw the overlays corresponding to the image displayed on the viewer, not the current timeline's time
         */
        double time = getCurrentlyDisplayedTime();
        _imp->viewerTab->drawOverlays(time, RenderScale(scale));
        
        glCheckErrorIgnoreOSXBug();

        if (_imp->pickerState == ePickerStateRectangle) {
            if ( _imp->viewerTab->getGui()->hasPickers() ) {
                drawPickerRectangle();
            }
        } else if (_imp->pickerState == ePickerStatePoint) {
            if ( _imp->viewerTab->getGui()->hasPickers() ) {
                drawPickerPixel();
            }
        }
        
        if (_imp->isUpdatingTexture) {
            glBegin(GL_LINES);
            glVertex2d(_imp->lastTextureRoi.x2, _imp->lastTextureRoi.y2);
            glVertex2d(_imp->lastTextureRoi.x2, _imp->lastTextureRoi.y1 - 1);
            glEnd();
        }

    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError();
    
    if (_imp->displayPersistentMessage) {
        drawPersistentMessage();
    }
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
            if (_imp->ms == eMouseStateBuildingUserRoI ||
                _imp->buildUserRoIOnNextPress) {
                userRoI = _imp->draggedUserRoI;
            } else {
                userRoI = _imp->userRoI;
            }
        }
        
        if (_imp->buildUserRoIOnNextPress) {
            glLineStipple(2, 0xAAAA);
            glEnable(GL_LINE_STIPPLE);
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
        
        if (_imp->buildUserRoIOnNextPress) {
            glDisable(GL_LINE_STIPPLE);
        }
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

    alphaMix1 = wipeAngle + M_PI_4 / 2;
    alphaMix0 = wipeAngle + 3. * M_PI_4 / 2;
    alphaCurMix = mixAmount * (alphaMix1 - alphaMix0) + alphaMix0;
    QPointF mix0Pos,mixPos,mix1Pos;
    
    double mixX,mixY,rotateW,rotateH,rotateOffsetX,rotateOffsetY;
    
    double zoomScreenPixelWidth, zoomScreenPixelHeight;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }

    mixX = WIPE_MIX_HANDLE_LENGTH * zoomScreenPixelWidth;
    mixY = WIPE_MIX_HANDLE_LENGTH * zoomScreenPixelHeight;
    rotateW = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelWidth;
    rotateH = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelHeight;
    rotateOffsetX = WIPE_ROTATE_OFFSET * zoomScreenPixelWidth;
    rotateOffsetY = WIPE_ROTATE_OFFSET * zoomScreenPixelHeight;


    mixPos.setX(wipeCenter.x() + std::cos(alphaCurMix) * mixX);
    mixPos.setY(wipeCenter.y() + std::sin(alphaCurMix) * mixY);
    mix0Pos.setX(wipeCenter.x() + std::cos(alphaMix0) * mixX);
    mix0Pos.setY(wipeCenter.y() + std::sin(alphaMix0) * mixY);
    mix1Pos.setX(wipeCenter.x() + std::cos(alphaMix1) * mixX);
    mix1Pos.setY(wipeCenter.y() + std::sin(alphaMix1) * mixY);

    QPointF oppositeAxisBottom,oppositeAxisTop,rotateAxisLeft,rotateAxisRight;
    rotateAxisRight.setX( wipeCenter.x() + std::cos(wipeAngle) * (rotateW - rotateOffsetX) );
    rotateAxisRight.setY( wipeCenter.y() + std::sin(wipeAngle) * (rotateH - rotateOffsetY) );
    rotateAxisLeft.setX(wipeCenter.x() - std::cos(wipeAngle) * rotateOffsetX);
    rotateAxisLeft.setY( wipeCenter.y() - (std::sin(wipeAngle) * rotateOffsetY) );

    oppositeAxisTop.setX( wipeCenter.x() + std::cos(wipeAngle + M_PI_2) * (rotateW / 2.) );
    oppositeAxisTop.setY( wipeCenter.y() + std::sin(wipeAngle + M_PI_2) * (rotateH / 2.) );
    oppositeAxisBottom.setX( wipeCenter.x() - std::cos(wipeAngle + M_PI_2) * (rotateW / 2.) );
    oppositeAxisBottom.setY( wipeCenter.y() - std::sin(wipeAngle + M_PI_2) * (rotateH / 2.) );

    {
        GLProtectAttrib a(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)

        // Draw everything twice
        // l = 0: shadow
        // l = 1: drawing
        double baseColor[3];
        for (int l = 0; l < 2; ++l) {
            
            // shadow (uses GL_PROJECTION)
            glMatrixMode(GL_PROJECTION);
            int direction = (l == 0) ? 1 : -1;
            // translate (1,-1) pixels
            glTranslated(direction * _imp->glShadow.x(), -direction * _imp->glShadow.x(), 0);
            glMatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke
            
            if (l == 0) {
                // Draw a shadow for the cross hair
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
            if ( (_imp->hs == eHoverStateWipeRotateHandle) || (_imp->ms == eMouseStateRotatingWipeHandle) ) {
                glColor4f(0., 1. * l, 0., 1.);
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
            if ( (_imp->hs == eHoverStateWipeRotateHandle) || (_imp->ms == eMouseStateRotatingWipeHandle) ) {
                GLProtectMatrix p(GL_MODELVIEW);

                glColor4f(0., 1. * l, 0., 1.);
                double arrowCenterX = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelWidth / 2;
                ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
                OfxPointD arrowRadius;
                arrowRadius.x = 5. * zoomScreenPixelWidth;
                arrowRadius.y = 10. * zoomScreenPixelHeight;

                glTranslatef(wipeCenter.x(), wipeCenter.y(), 0.);
                glRotatef(wipeAngle * 180.0 / M_PI, 0, 0, 1);
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
                glVertex2f(4. * zoomScreenPixelWidth, arrowRadius.y - 3. * zoomScreenPixelHeight); // 5^2 = 3^2+4^2

                ///draw the bottom head
                glVertex2f(0., -arrowRadius.y);
                glVertex2f(0., -arrowRadius.y + 5. * zoomScreenPixelHeight);

                glVertex2f(0., -arrowRadius.y);
                glVertex2f(4. * zoomScreenPixelWidth, -arrowRadius.y + 3. * zoomScreenPixelHeight); // 5^2 = 3^2+4^2

                glEnd();

                glColor4f(baseColor[0],baseColor[1],baseColor[2],1.);
            }

            glPointSize(5.);
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2d( wipeCenter.x(), wipeCenter.y() );
            if ( ((_imp->hs == eHoverStateWipeMix) &&
                  (_imp->ms != eMouseStateRotatingWipeHandle) )
                || (_imp->ms == eMouseStateDraggingWipeMixHandle) ) {
                glColor4f(0., 1. * l, 0., 1.);
            }
            glVertex2d( mixPos.x(), mixPos.y() );
            glEnd();
            glPointSize(1.);
            
            _imp->drawArcOfCircle(wipeCenter, mixX, mixY, wipeAngle + M_PI_4 / 2, wipeAngle + 3. * M_PI_4 / 2);
      
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
} // drawWipeControl


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

namespace {

    
static QStringList explode(const QString& str)
{
    QRegExp rx("(\\ |\\-|\\.|\\/|\\t|\\n)"); //RegEx for ' ' '/' '.' '-' '\t' '\n'

    QStringList ret;
    int startIndex = 0;
    while (true) {
        int index = str.indexOf(rx, startIndex);
        
        if (index == -1) {
            ret.push_back(str.mid(startIndex));
            return ret;
        }
        
        QString word = str.mid(startIndex, index - startIndex);
        
        const QChar& nextChar = str[index];
        
        // Dashes and the likes should stick to the word occuring before it. Whitespace doesn't have to.
        if (nextChar.isSpace()) {
            ret.push_back(word);
            ret.push_back(nextChar);
        } else {
            ret.push_back(word + nextChar);
        }
        
        startIndex = index + 1;
    }
    return ret;
}
    
static QStringList wordWrap(const QFontMetrics& fm,const QString& str, int width)
{
    QStringList words = explode(str);
    
    int curLineLength = 0;

    QStringList stringL;
    QString curString;
    
    for(int i = 0; i < words.size(); ++i) {
        
        QString word = words[i];
        int wordPixels = fm.width(word);
        
        // If adding the new word to the current line would be too long,
        // then put it on a new line (and split it up if it's too long).
        if (curLineLength + wordPixels > width) {
            // Only move down to a new line if we have text on the current line.
            // Avoids situation where wrapped whitespace causes emptylines in text.
            if (curLineLength > 0) {
                if (!curString.isEmpty()) {
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
                curString.append(word.mid(0, width - 1));
                word = word.mid(width - 1);
                
                if (!curString.isEmpty()) {
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
    if (!curString.isEmpty()) {
        stringL.push_back(curString);
    }
    return stringL;
}

}

void
ViewerGL::drawPersistentMessage()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    QFontMetrics metrics( *_imp->textFont );

    int offset =  10;
    double metricsHeightZoomCoord;
    QPointF topLeft, bottomRight,offsetZoomCoord;
    
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        topLeft = _imp->zoomCtx.toZoomCoordinates(0,0);
        bottomRight = _imp->zoomCtx.toZoomCoordinates( _imp->zoomCtx.screenWidth(),_imp->persistentMessages.size() * (metrics.height() + offset) );
        offsetZoomCoord = _imp->zoomCtx.toZoomCoordinates(PERSISTENT_MESSAGE_LEFT_OFFSET_PIXELS, offset);
        metricsHeightZoomCoord = topLeft.y() - _imp->zoomCtx.toZoomCoordinates(0, metrics.height()).y();
    }
    offsetZoomCoord.ry() = topLeft.y() - offsetZoomCoord.y();
    QPointF textPos(offsetZoomCoord.x(),  topLeft.y() - (offsetZoomCoord.y() / 2.) - metricsHeightZoomCoord);
    
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


        for (int j = 0; j < _imp->persistentMessages.size(); ++j) {
            renderText(textPos.x(),textPos.y(), _imp->persistentMessages.at(j),_imp->textRenderingColor,*_imp->textFont);
            textPos.setY(textPos.y() - (metricsHeightZoomCoord + offsetZoomCoord.y()));/*metrics.height() * 2 * zoomScreenPixelHeight*/
        }
        glCheckError();
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
} // drawPersistentMessage


void
ViewerGL::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();
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
                                     const double par,
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
        visibleArea.toPixelEnclosing(mipMapLevel,par,&ret);
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
        userRoI.toPixelEnclosing(mipMapLevel,par, &userRoIpixel);

        ///If the user roi doesn't intersect the actually visible portion on the viewer, return an empty rectangle.
        if ( !ret.intersect(userRoIpixel, &ret) ) {
            ret.clear();
        }
    }

    return ret;
}

RectI
ViewerGL::getExactImageRectangleDisplayed(const RectD & rod,const double par,unsigned int mipMapLevel)
{
    bool clipToProject = isClippingImageToProjectWindow();
    RectD clippedRod;
    
    if (clipToProject) {
        RectD projectFormatCanonical;
        _imp->getProjectFormatCanonical(projectFormatCanonical);
        rod.intersect(projectFormatCanonical,&clippedRod);
    } else {
        clippedRod = rod;
    }
    
    RectI bounds;
    clippedRod.toPixelEnclosing(mipMapLevel, par, &bounds);
    RectI roi = getImageRectangleDisplayed(bounds, par, mipMapLevel);
    return roi;
}

RectI
ViewerGL::getImageRectangleDisplayedRoundedToTileSize(const RectD & rod,const double par,unsigned int mipMapLevel)
{
    bool clipToProject = isClippingImageToProjectWindow();
    RectD clippedRod;
    
    if (clipToProject) {
        RectD projectFormatCanonical;
        _imp->getProjectFormatCanonical(projectFormatCanonical);
        rod.intersect(projectFormatCanonical,&clippedRod);
    } else {
        clippedRod = rod;
    }
    
    RectI bounds;
    clippedRod.toPixelEnclosing(mipMapLevel, par, &bounds);
    RectI roi = getImageRectangleDisplayed(bounds, par, mipMapLevel);
    
    ////Texrect is the coordinates of the 4 corners of the texture in the bounds with the current zoom
    ////factor taken into account.
    RectI texRect;
    double tileSize = std::pow( 2., (double)appPTR->getCurrentSettings()->getViewerTilesPowerOf2() );
    texRect.x1 = std::floor( ( (double)roi.x1 ) / tileSize ) * tileSize;
    texRect.y1 = std::floor( ( (double)roi.y1 ) / tileSize ) * tileSize;
    texRect.x2 = std::ceil( ( (double)roi.x2 ) / tileSize ) * tileSize;
    texRect.y2 = std::ceil( ( (double)roi.y2 ) / tileSize ) * tileSize;
    
    ///Make sure the bounds of the area to render in the texture lies in the bounds
    texRect.intersect(bounds, &texRect);

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
ViewerGL::initShaderGLSL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if (!_imp->shaderLoaded && _imp->supportsGLSL) {
        _imp->shaderBlack = new QGLShaderProgram( context() );
        if ( !_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Vertex, vertRGB) ) {
            cout << qPrintable( _imp->shaderBlack->log() ) << endl;
        }
        if ( !_imp->shaderBlack->addShaderFromSourceCode(QGLShader::Fragment, blackFrag) ) {
            cout << qPrintable( _imp->shaderBlack->log() ) << endl;
        }
        if ( !_imp->shaderBlack->link() ) {
            cout << qPrintable( _imp->shaderBlack->log() ) << endl;
        }

        _imp->shaderRGB = new QGLShaderProgram( context() );
        if ( !_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Vertex, vertRGB) ) {
            cout << qPrintable( _imp->shaderRGB->log() ) << endl;
        }
        if ( !_imp->shaderRGB->addShaderFromSourceCode(QGLShader::Fragment, fragRGB) ) {
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
                                     const std::list<boost::shared_ptr<Natron::Image> >& tiles,
                                     ImageBitDepthEnum depth,
                                     int time,
                                     const RectD& rod,
                                     size_t bytesCount,
                                     const TextureRect & region,
                                     double gain,
                                     double gamma, 
                                     double offset,
                                     int lut,
                                     int pboIndex,
                                     unsigned int mipMapLevel,
                                     ImagePremultiplicationEnum premult,
                                     int textureIndex,
                                     const RectI& roi,
                                     bool updateOnlyRoi)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    GLenum e = glGetError();
    Q_UNUSED(e);
    
    GLint currentBoundPBO = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    GLenum err = glGetError();
    if ( (err != GL_NO_ERROR) || (currentBoundPBO != 0) ) {
        qDebug() << "(ViewerGL::allocateAndMapPBO): Another PBO is currently mapped, glMap failed." << endl;
    }
    
    GLuint pboId = getPboID(pboIndex);
    
    ImageBitDepthEnum bd = getBitDepth();
    assert(textureIndex == 0 || textureIndex == 1);
    
    if (updateOnlyRoi) {
        //Make sure the texture is allocated on the full portion
        Texture::DataTypeEnum type = Texture::eDataTypeNone;
        switch (bd) {
            case Natron::eImageBitDepthByte: {
                type = Texture::eDataTypeByte;
                break;
            }
            case Natron::eImageBitDepthFloat: {
                type = Texture::eDataTypeFloat;
                //do 32bit fp textures either way, don't bother with half float. We might support it one day.
                break;
            }
            default:
                throw std::logic_error("ViewerGL::transferBufferFromRAMtoGPU(): unknown texture type");
        }
        if (_imp->displayTextures[textureIndex]->mustAllocTexture(region)) {
            ///Initialize with black and transparant
            std::size_t bytesToInit = region.w * region.h * 4;
            if (depth == eImageBitDepthFloat) {
                bytesToInit *= sizeof(float);
            }
            glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboId );
            glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesToInit, NULL, GL_DYNAMIC_DRAW_ARB);
            GLvoid *ret = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
            glCheckError();
            assert(ret);
            memset(ret, 0, bytesToInit);
            glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
            glCheckError();
            _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region, type, roi, false);
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);
            glCheckError();
        }
    }
    
    _imp->isUpdatingTexture = updateOnlyRoi;
    if (updateOnlyRoi) {
        roi.toCanonical_noClipping(mipMapLevel, 1., &_imp->lastTextureRoi);
    }
    
    

    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboId );
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);
    GLvoid *ret = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError();
    assert(ret);

    memcpy(ret, (void*)ramBuffer, bytesCount);

    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glCheckError();

    
    if (bd == Natron::eImageBitDepthByte) {
        _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region, Texture::eDataTypeByte, roi, updateOnlyRoi);
    } else if (bd == Natron::eImageBitDepthFloat) {
        //do 32bit fp textures either way, don't bother with half float. We might support it further on.
        _imp->displayTextures[textureIndex]->fillOrAllocateTexture(region, Texture::eDataTypeFloat, roi, updateOnlyRoi);
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);
    //glBindTexture(GL_TEXTURE_2D, 0); // why should we bind texture 0?
    glCheckError();
    _imp->activeTextures[textureIndex] = _imp->displayTextures[textureIndex];
    _imp->displayingImageGain[textureIndex] = gain;
    _imp->displayingImageGamma[textureIndex] = gamma;
    _imp->displayingImageOffset[textureIndex] = offset;
    _imp->displayingImageMipMapLevel[textureIndex] = mipMapLevel;
    _imp->displayingImageLut = (ViewerColorSpaceEnum)lut;
    _imp->displayingImagePremult[textureIndex] = premult;
    _imp->displayingImageTime[textureIndex] = time;
    ViewerInstance* internalNode = getInternalNode();
    
    if (_imp->memoryHeldByLastRenderedImages[textureIndex] > 0) {
        internalNode->unregisterPluginMemory(_imp->memoryHeldByLastRenderedImages[textureIndex]);
        _imp->memoryHeldByLastRenderedImages[textureIndex] = 0;
    }
    


    if (!tiles.empty() && tiles.front()) {
        const ImagePtr& firstTile = tiles.front();
        _imp->viewerTab->setImageFormat(textureIndex, firstTile->getComponents(), depth);
        RectI pixelRoD;
        firstTile->getRoD().toPixelEnclosing(0, firstTile->getPixelAspectRatio(), &pixelRoD);
        {
            QMutexLocker k(&_imp->projectFormatMutex);
            _imp->currentViewerInfo[textureIndex].setDisplayWindow(Format(_imp->projectFormat, firstTile->getPixelAspectRatio()));
        }
        {
            QMutexLocker k(&_imp->lastRenderedImageMutex);
            _imp->lastRenderedTiles[textureIndex][mipMapLevel] = tiles;
        }
        _imp->memoryHeldByLastRenderedImages[textureIndex] = 0;
        for (ImageList::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            _imp->memoryHeldByLastRenderedImages[textureIndex] += (*it)->size();
        }
        internalNode->registerPluginMemory(_imp->memoryHeldByLastRenderedImages[textureIndex]);
        Q_EMIT imageChanged(textureIndex,true);
    } else {
        if (_imp->lastRenderedTiles[textureIndex][mipMapLevel].empty()) {
            Q_EMIT imageChanged(textureIndex,false);
        } else {
            Q_EMIT imageChanged(textureIndex,true);
        }
    }
    setRegionOfDefinition(rod,region.par,textureIndex);

}

void
ViewerGL::clearLastRenderedImage()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    ViewerInstance* internalNode = getInternalNode();

    for (int i = 0; i < 2; ++i) {
        for (U32 j = 0; j < _imp->lastRenderedTiles[i].size(); ++j) {
            _imp->lastRenderedTiles[i][j].clear();
        }
        if (_imp->memoryHeldByLastRenderedImages[i] > 0) {
            internalNode->unregisterPluginMemory(_imp->memoryHeldByLastRenderedImages[i]);
            _imp->memoryHeldByLastRenderedImages[i] = 0;
        }
    }
}

void
ViewerGL::disconnectInputTexture(int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(textureIndex == 0 || textureIndex == 1);
    if (_imp->activeTextures[textureIndex] != 0) {
        _imp->activeTextures[textureIndex] = 0;
        RectI r(0,0,0,0);
        _imp->infoViewer[textureIndex]->setDataWindow(r);
    }
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
ViewerGL::setGamma(double g)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->displayingImageGamma[0] = g;
    _imp->displayingImageGamma[1] = g;
}

void
ViewerGL::setLut(int lut)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->displayingImageLut = (ViewerColorSpaceEnum)lut;
}

/**
 *@returns Returns true if the graphic card supports GLSL.
 **/
bool
ViewerGL::supportsGLSL() const
{
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
    
    _imp->viewerTab->onMousePressCalledInViewer();

    _imp->hasMovedSincePress = false;
    _imp->pressureOnRelease = 1.;

    ///Set focus on user click
    setFocus();

    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::MouseButton button = e->button();

    if ( buttonDownIsLeft(e) ) {

        boost::shared_ptr<NodeGuiI> gui_i = _imp->viewerTab->getInternalNode()->getNode()->getNodeGui();
        assert(gui_i);
        boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
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
    RectD userRoI;
    bool userRoIEnabled;
    {
        QMutexLocker l(&_imp->userRoIMutex);
        userRoI = _imp->userRoI;
        userRoIEnabled = _imp->userRoIEnabled;
    }
    bool overlaysCaught = false;
    bool mustRedraw = false;

    bool hasPickers = _imp->viewerTab->getGui()->hasPickers();

    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        _imp->buildUserRoIOnNextPress) {
        _imp->draggedUserRoI.x1 = zoomPos.x();
        _imp->draggedUserRoI.y1 = zoomPos.y();
        _imp->draggedUserRoI.x2 = _imp->draggedUserRoI.x1 + 1;
        _imp->draggedUserRoI.y2 = _imp->draggedUserRoI.y1 + 1;
        _imp->buildUserRoIOnNextPress = false;
        _imp->ms = eMouseStateBuildingUserRoI;
        overlaysCaught = true;
        mustRedraw = true;
    }
    
    if (!overlaysCaught &&
        (buttonDownIsMiddle(e) ||
         ( (e)->buttons() == Qt::RightButton && buttonMetaAlt(e) == Qt::AltModifier )) &&
        !modifierHasControl(e) ) {
        // middle (or Alt + left) or Alt + right = pan
        _imp->ms = eMouseStateDraggingImage;
        overlaysCaught = true;
    }
    if (!overlaysCaught &&
        (((e->buttons() & Qt::MiddleButton) &&
          (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
         ((e->buttons() & Qt::LeftButton) &&
          (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier))))) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->ms = eMouseStateZoomingImage;
        overlaysCaught = true;
    }
    if (!overlaysCaught &&
        hasPickers &&
        isMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickColor, modifiers, button) &&
        displayingImage() ) {
        // picker with single-point selection
        _imp->pickerState = ePickerStatePoint;
        if ( pickColor( e->x(),e->y() ) ) {
            _imp->ms = eMouseStatePickingColor;
            mustRedraw = true;
            overlaysCaught = true;
        }
    }

    // process the wipe tool events before the plugin overlays
    if (!overlaysCaught &&
        _imp->overlay &&
        isWipeHandleVisible() &&
        buttonDownIsLeft(e) &&
        _imp->isNearbyWipeCenter(zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        _imp->ms = eMouseStateDraggingWipeCenter;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        _imp->overlay &&
        isWipeHandleVisible() &&
        buttonDownIsLeft(e) &&
        _imp->isNearbyWipeMixHandle(zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        _imp->ms = eMouseStateDraggingWipeMixHandle;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        _imp->overlay &&
        isWipeHandleVisible() &&
        buttonDownIsLeft(e) &&
        _imp->isNearbyWipeRotateBar(zoomPos, zoomScreenPixelWidth ,zoomScreenPixelHeight) ) {
        _imp->ms = eMouseStateRotatingWipeHandle;
        overlaysCaught = true;
        mustRedraw = true;
    }

    // process plugin overlays
    if (!overlaysCaught &&
        (_imp->ms == eMouseStateUndefined) &&
        _imp->overlay ) {
        unsigned int mipMapLevel = getCurrentRenderScale();
        double scale = 1. / (1 << mipMapLevel);
        overlaysCaught = _imp->viewerTab->notifyOverlaysPenDown(RenderScale(scale), _imp->pointerTypeOnPress, _imp->subsequentMousePressIsTablet, QMouseEventLocalPos(e), zoomPos, _imp->pressureOnPress, currentTimeForEvent(e), e);
        if (overlaysCaught) {
            mustRedraw = true;
        }
    }


    if (!overlaysCaught &&
        hasPickers &&
        isMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseRectanglePick, modifiers, button) &&
        displayingImage() ) {
        // start picker with rectangle selection (picked color is the average over the rectangle)
        _imp->pickerState = ePickerStateRectangle;
        _imp->pickerRect.setTopLeft(zoomPos);
        _imp->pickerRect.setBottomRight(zoomPos);
        _imp->ms = eMouseStateBuildingPickerRectangle;
        mustRedraw = true;
        overlaysCaught = true;
    }
    if (!overlaysCaught &&
        (_imp->pickerState != ePickerStateInactive) &&
        buttonDownIsLeft(e) &&
        displayingImage() ) {
        // disable picker if picker is set when clicking
        _imp->pickerState = ePickerStateInactive;
        mustRedraw = true;
        overlaysCaught = true;
    }
    
    if (hasPickers && _imp->pickerState == ePickerStateInactive) {
        _imp->viewerTab->getGui()->clearColorPickers();
        unsetCursor();
    }
    
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoIBottomEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the bottom edge of the user ROI
        _imp->ms = eMouseStateDraggingRoiBottomEdge;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoILeftEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the left edge of the user ROI
        _imp->ms = eMouseStateDraggingRoiLeftEdge;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled && isNearByUserRoIRightEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the right edge of the user ROI
        _imp->ms = eMouseStateDraggingRoiRightEdge;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoITopEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the top edge of the user ROI
        _imp->ms = eMouseStateDraggingRoiTopEdge;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoI( (userRoI.x1 + userRoI.x2) / 2., (userRoI.y1 + userRoI.y2) / 2.,
                        zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight ) ) {
            // start dragging the midpoint of the user ROI
            _imp->ms = eMouseStateDraggingRoiCross;
            overlaysCaught = true;
            mustRedraw = true;
        }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoI(userRoI.x1, userRoI.y2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the topleft corner of the user ROI
        _imp->ms = eMouseStateDraggingRoiTopLeft;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoI(userRoI.x2, userRoI.y2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the topright corner of the user ROI
        _imp->ms = eMouseStateDraggingRoiTopRight;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoI(userRoI.x1, userRoI.y1, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the bottomleft corner of the user ROI
        _imp->ms = eMouseStateDraggingRoiBottomLeft;
        overlaysCaught = true;
        mustRedraw = true;
    }
    if (!overlaysCaught &&
        buttonDownIsLeft(e) &&
        userRoIEnabled &&
        isNearByUserRoI(userRoI.x2, userRoI.y1, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
        // start dragging the bottomright corner of the user ROI
        _imp->ms = eMouseStateDraggingRoiBottomRight;
        overlaysCaught = true;
        mustRedraw = true;
    }

    if (!overlaysCaught &&
        buttonDownIsRight(e) ) {
        _imp->menu->exec( mapToGlobal( e->pos() ) );
        overlaysCaught = true;
    }
    if (!overlaysCaught &&
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

    if (!_imp->viewerTab->getGui()) {
        return;
    }

    _imp->pressureOnPress = 1;
    _imp->subsequentMousePressIsTablet = false;

    bool mustRedraw = false;
    if (_imp->ms == eMouseStateBuildingPickerRectangle) {
        updateRectangleColorPicker();
    }

    if (_imp->ms == eMouseStateSelecting) {
        mustRedraw = true;

        if (_imp->hasMovedSincePress) {
            Q_EMIT selectionRectangleChanged(true);
        }
    } else if (_imp->ms == eMouseStateBuildingUserRoI) {
        QMutexLocker k(&_imp->userRoIMutex);
        _imp->userRoI = _imp->draggedUserRoI;
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
    if ( _imp->viewerTab->notifyOverlaysPenUp(RenderScale(scale), QMouseEventLocalPos(e), zoomPos, currentTimeForEvent(e), _imp->pressureOnRelease, e) ) {
        mustRedraw = true;
    }
    if (mustRedraw) {
        update();
    }
    
    if (_imp->renderOnPenUp) {
        _imp->renderOnPenUp = false;
        getInternalNode()->renderCurrentFrame(false);
    }
}

void
ViewerGL::mouseMoveEvent(QMouseEvent* e)
{
    if (!penMotionInternal(e->x(), e->y(), /*pressure=*/1., currentTimeForEvent(e), e)) {
        QGLWidget::mouseMoveEvent(e);
    }
} // mouseMoveEvent

void
ViewerGL::tabletEvent(QTabletEvent* e)
{

    switch (e->type()) {
    case QEvent::TabletPress: {
        switch (e->pointerType()) {
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
        _imp->subsequentMousePressIsTablet = true;
        QGLWidget::tabletEvent(e);
    }   break;
    case QEvent::TabletRelease: {
        _imp->pressureOnRelease = e->pressure();
        QGLWidget::tabletEvent(e);
    }   break;
    case QEvent::TabletMove: {
        if (!penMotionInternal(e->x(), e->y(), e->pressure(), currentTimeForEvent(e), e)) {
            QGLWidget::tabletEvent(e);
        } else {
            e->accept();
        }
    }   break;
    default:
        break;
    }

}

bool
ViewerGL::penMotionInternal(int x, int y, double pressure, double timestamp, QInputEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    ///The app is closing don't do anything
    Gui* gui = _imp->viewerTab->getGui();
    if ( !gui || !getInternalNode()) {
        return false;
    }
    
    _imp->hasMovedSincePress = true;
    
    QPointF zoomPos;
    
    // if the picker was deselected, this fixes the picer State
    // (see issue #133 https://github.com/MrKepzie/Natron/issues/133 )
    if ( !gui->hasPickers() ) {
        _imp->pickerState = ePickerStateInactive;
    }
    
    double zoomScreenPixelWidth, zoomScreenPixelHeight; // screen pixel size in zoom coordinates
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        zoomPos = _imp->zoomCtx.toZoomCoordinates(x, y);
        zoomScreenPixelWidth = _imp->zoomCtx.screenPixelWidth();
        zoomScreenPixelHeight = _imp->zoomCtx.screenPixelHeight();
    }
    
    updateInfoWidgetColorPicker(zoomPos, QPoint(x,y));
    if (_imp->viewerTab->isViewersSynchroEnabled()) {
        const std::list<ViewerTab*>& allViewers = gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = allViewers.begin(); it!=allViewers.end(); ++it) {
            if ((*it)->getViewer() != this) {
                (*it)->getViewer()->updateInfoWidgetColorPicker(zoomPos, QPoint(x,y));
            }
        }
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
    bool wasHovering = _imp->hs != eHoverStateNothing;
    
    bool cursorSet = false;
    if ( (_imp->ms != eMouseStateDraggingImage) && _imp->overlay) {
        _imp->hs = eHoverStateNothing;
        if ( isWipeHandleVisible() && _imp->isNearbyWipeCenter(zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            setCursor( QCursor(Qt::SizeAllCursor) );
            cursorSet = true;
        } else if ( isWipeHandleVisible() && _imp->isNearbyWipeMixHandle(zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            _imp->hs = eHoverStateWipeMix;
            mustRedraw = true;
        } else if ( isWipeHandleVisible() && _imp->isNearbyWipeRotateBar(zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ) {
            _imp->hs = eHoverStateWipeRotateHandle;
            mustRedraw = true;
        } else if (userRoIEnabled) {
            if ( isNearByUserRoIBottomEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                || isNearByUserRoITopEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                || ( _imp->ms == eMouseStateDraggingRoiBottomEdge)
                || ( _imp->ms == eMouseStateDraggingRoiTopEdge) ) {
                setCursor( QCursor(Qt::SizeVerCursor) );
                cursorSet = true;
            } else if ( isNearByUserRoILeftEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                       || isNearByUserRoIRightEdge(userRoI,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight)
                       || ( _imp->ms == eMouseStateDraggingRoiLeftEdge)
                       || ( _imp->ms == eMouseStateDraggingRoiRightEdge) ) {
                setCursor( QCursor(Qt::SizeHorCursor) );
                cursorSet = true;
            } else if ( isNearByUserRoI( (userRoI.x1 + userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight )
                       || ( _imp->ms == eMouseStateDraggingRoiCross) ) {
                setCursor( QCursor(Qt::SizeAllCursor) );
                cursorSet = true;
            } else if ( isNearByUserRoI(userRoI.x2, userRoI.y1, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                       isNearByUserRoI(userRoI.x1, userRoI.y2, zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                       ( _imp->ms == eMouseStateDraggingRoiBottomRight) ||
                       ( _imp->ms == eMouseStateDraggingRoiTopLeft) ) {
                setCursor( QCursor(Qt::SizeFDiagCursor) );
                cursorSet = true;
            } else if ( isNearByUserRoI(userRoI.x1, userRoI.y1,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                       isNearByUserRoI(userRoI.x2, userRoI.y2,zoomPos, zoomScreenPixelWidth, zoomScreenPixelHeight) ||
                       ( _imp->ms == eMouseStateDraggingRoiBottomLeft) ||
                       ( _imp->ms == eMouseStateDraggingRoiTopRight) ) {
                setCursor( QCursor(Qt::SizeBDiagCursor) );
                cursorSet = true;
            }
        }
    }
    
    if ( (_imp->hs == eHoverStateNothing) && wasHovering ) {
        mustRedraw = true;
    }
    
    QPoint newClick(x,y);
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
    bool overlaysCaughtByPlugin = false;
    switch (_imp->ms) {
        case eMouseStateDraggingImage: {
            {
                QMutexLocker l(&_imp->zoomCtxMutex);
                _imp->zoomCtx.translate(dx, dy);
                _imp->zoomOrPannedSinceLastFit = true;
            }
            _imp->oldClick = newClick;
            
            if (_imp->viewerTab->isViewersSynchroEnabled()) {
                _imp->viewerTab->synchronizeOtherViewersProjection();
            }
            
            _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
            
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
            int delta = 2*((x - _imp->lastMousePosition.x()) - (y - _imp->lastMousePosition.y()));
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
                zoomValue = 1; // sometimes, floor(100*0.01) makes 0
            }
            assert(zoomValue > 0);
            Q_EMIT zoomChanged(zoomValue);
            
            if (_imp->viewerTab->isViewersSynchroEnabled()) {
                _imp->viewerTab->synchronizeOtherViewersProjection();
            }
            
            //_imp->oldClick = newClick; // don't update oldClick! this is the zoom center
            _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
            
            //  else {
            mustRedraw = true;
            // }
            // no need to update the color picker or mouse posn: they should be unchanged
            break;
        }
        case eMouseStateDraggingRoiBottomEdge: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
                l.unlock();
                if ( displayingImage() ) {
                    _imp->renderOnPenUp = true;
                }
                mustRedraw = true;
            }
            break;
        }
        case eMouseStateDraggingRoiLeftEdge: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
                l.unlock();
                if ( displayingImage() ) {
                    _imp->renderOnPenUp = true;
                }
                mustRedraw = true;
            }
            break;
        }
        case eMouseStateDraggingRoiRightEdge: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
                l.unlock();
                if ( displayingImage() ) {
                    _imp->renderOnPenUp = true;
                }
                mustRedraw = true;
            }
            break;
        }
        case eMouseStateDraggingRoiTopEdge: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
                l.unlock();
                if ( displayingImage() ) {
                    _imp->renderOnPenUp = true;
                }
                mustRedraw = true;
            }
            break;
        }
        case eMouseStateDraggingRoiCross: {
            {
                QMutexLocker l(&_imp->userRoIMutex);
                _imp->userRoI.translate(-dxSinceLastMove,-dySinceLastMove);
            }
            if ( displayingImage() ) {
                _imp->renderOnPenUp = true;
            }
            mustRedraw = true;
            break;
        }
        case eMouseStateDraggingRoiTopLeft: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
            }
            if ( (_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
            }
            l.unlock();
            if ( displayingImage() ) {
                _imp->renderOnPenUp = true;
            }
            mustRedraw = true;
            break;
        }
        case eMouseStateDraggingRoiTopRight: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.y2 - dySinceLastMove) > _imp->userRoI.y1 ) {
                _imp->userRoI.y2 -= dySinceLastMove;
            }
            if ( (_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
            }
            l.unlock();
            if ( displayingImage() ) {
                _imp->renderOnPenUp = true;
            }
            mustRedraw = true;
            break;
        }
        case eMouseStateDraggingRoiBottomRight: {
            QMutexLocker l(&_imp->userRoIMutex);
            if ( (_imp->userRoI.x2 - dxSinceLastMove) > _imp->userRoI.x1 ) {
                _imp->userRoI.x2 -= dxSinceLastMove;
            }
            if ( (_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
            }
            l.unlock();
            if ( displayingImage() ) {
                _imp->renderOnPenUp = true;
            }
            mustRedraw = true;
            break;
        }
        case eMouseStateDraggingRoiBottomLeft: {
            if ( (_imp->userRoI.y1 - dySinceLastMove) < _imp->userRoI.y2 ) {
                _imp->userRoI.y1 -= dySinceLastMove;
            }
            if ( (_imp->userRoI.x1 - dxSinceLastMove) < _imp->userRoI.x2 ) {
                _imp->userRoI.x1 -= dxSinceLastMove;
            }
            _imp->userRoIMutex.unlock();
            if ( displayingImage() ) {
                _imp->renderOnPenUp = true;
            }
            mustRedraw = true;
            break;
        }
        case eMouseStateBuildingUserRoI: {
            if ( (_imp->draggedUserRoI.x2 - dxSinceLastMove) > _imp->draggedUserRoI.x1 ) {
                _imp->draggedUserRoI.x2 -= dxSinceLastMove;
            }
            if ( (_imp->draggedUserRoI.y1 - dySinceLastMove) < _imp->draggedUserRoI.y2 ) {
                _imp->draggedUserRoI.y1 -= dySinceLastMove;
            }
            if ( displayingImage() ) {
                _imp->renderOnPenUp = true;
            }
            mustRedraw = true;
            break;
        }
        case eMouseStateDraggingWipeCenter: {
            QMutexLocker l(&_imp->wipeControlsMutex);
            _imp->wipeCenter.rx() -= dxSinceLastMove;
            _imp->wipeCenter.ry() -= dySinceLastMove;
            mustRedraw = true;
            break;
        }
        case eMouseStateDraggingWipeMixHandle: {
            QMutexLocker l(&_imp->wipeControlsMutex);
            double angle = std::atan2( zoomPos.y() - _imp->wipeCenter.y(), zoomPos.x() - _imp->wipeCenter.x() );
            double prevAngle = std::atan2( oldPosition_opengl.y() - _imp->wipeCenter.y(),
                                          oldPosition_opengl.x() - _imp->wipeCenter.x() );
            _imp->mixAmount -= (angle - prevAngle);
            _imp->mixAmount = std::max( 0.,std::min(_imp->mixAmount,1.) );
            mustRedraw = true;
            break;
        }
        case eMouseStateRotatingWipeHandle: {
            QMutexLocker l(&_imp->wipeControlsMutex);
            double angle = std::atan2( zoomPos.y() - _imp->wipeCenter.y(), zoomPos.x() - _imp->wipeCenter.x() );
            _imp->wipeAngle = angle;
            double closestPI2 = M_PI_2 * std::floor(_imp->wipeAngle / M_PI_2 + 0.5);
            if (std::fabs(_imp->wipeAngle - closestPI2) < 0.1) {
                // snap to closest multiple of PI / 2.
                _imp->wipeAngle = closestPI2;
            }
            
            mustRedraw = true;
            break;
        }
        case eMouseStatePickingColor: {
            pickColor( newClick.x(), newClick.y() );
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
            QPointF localPos(x,y);
            unsigned int mipMapLevel = getCurrentRenderScale();
            double scale = 1. / (1 << mipMapLevel);
            if ( _imp->overlay &&
                _imp->viewerTab->notifyOverlaysPenMotion(RenderScale(scale), localPos, zoomPos, pressure, timestamp, e) ) {
                mustRedraw = true;
                overlaysCaughtByPlugin = true;
            }
            break;
        }
    } // switch
    
    if (mustRedraw) {
        update();
    }
    _imp->lastMousePosition = newClick;
    if (!cursorSet) {
        if (_imp->viewerTab->getGui()->hasPickers()){
            setCursor(appPTR->getColorPickerCursor());
        } else if (!overlaysCaughtByPlugin) {
            unsetCursor();
        }
    }
    return true;
}

void
ViewerGL::mouseDoubleClickEvent(QMouseEvent* e)
{
    unsigned int mipMapLevel = getInternalNode()->getMipMapLevel();
    QPointF pos_opengl;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        pos_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(),e->y() );
    }
    double scale = 1. / (1 << mipMapLevel);
    if ( _imp->viewerTab->notifyOverlaysPenDoubleClick(RenderScale(scale), QMouseEventLocalPos(e), pos_opengl, e) ) {
        update();
    }
    QGLWidget::mouseDoubleClickEvent(e);
}

QPointF
ViewerGL::toZoomCoordinates(const QPointF& position) const
{
    QMutexLocker l(&_imp->zoomCtxMutex);
    return _imp->zoomCtx.toZoomCoordinates(position.x(), position.y());
}

// used to update the information bar at the bottom of the viewer (not for the ctrl-click color picker)
void
ViewerGL::updateColorPicker(int textureIndex,
                            int x,
                            int y)
{
    if (_imp->pickerState != ePickerStateInactive || !_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->isGUIFrozen()) {
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
    
    QPointF imgPosCanonical;
    if (!xInitialized || !yInitialized) {
        if (!_imp->viewerTab->isViewersSynchroEnabled()) {
            pos = mapFromGlobal(QCursor::pos());
            QMutexLocker l(&_imp->zoomCtxMutex);
            imgPosCanonical = _imp->zoomCtx.toZoomCoordinates(pos.x(), pos.y());
        } else {
            ViewerTab* masterViewer = getViewerTab()->getGui()->getMasterSyncViewer();
            if (masterViewer) {
                pos = masterViewer->getViewer()->mapFromGlobal(QCursor::pos());
                imgPosCanonical = masterViewer->getViewer()->toZoomCoordinates(pos);
            } else {
                pos = mapFromGlobal(QCursor::pos());
                QMutexLocker l(&_imp->zoomCtxMutex);
                imgPosCanonical = _imp->zoomCtx.toZoomCoordinates(pos.x(), pos.y());
            }
        }
    } else {
        QMutexLocker l(&_imp->zoomCtxMutex);
        imgPosCanonical = _imp->zoomCtx.toZoomCoordinates(pos.x(), pos.y());
    }

    float r,g,b,a;

    bool linear = appPTR->getCurrentSettings()->getColorPickerLinear();
    bool picked = false;
    RectD rod = getRoD(textureIndex);
    RectD projectCanonical;
    _imp->getProjectFormatCanonical(projectCanonical);
    unsigned int mmLevel;
    if ( ( imgPosCanonical.x() >= rod.left() ) &&
        ( imgPosCanonical.x() < rod.right() ) &&
        ( imgPosCanonical.y() >= rod.bottom() ) &&
        ( imgPosCanonical.y() < rod.top() )) {
        
        if ((pos.x() >= 0) && ( pos.x() < width()) &&
            (pos.y() >= 0) && ( pos.y() < height())) {
            
            ///if the clip to project format is enabled, make sure it is in the project format too
            bool clipping = isClippingImageToProjectWindow();
            if ( !clipping ||
                ( ( imgPosCanonical.x() >= projectCanonical.left() ) &&
                 ( imgPosCanonical.x() < projectCanonical.right() ) &&
                 ( imgPosCanonical.y() >= projectCanonical.bottom() ) &&
                 ( imgPosCanonical.y() < projectCanonical.top() ) ) ) {
                    //imgPos must be in canonical coordinates
                    picked = getColorAt(imgPosCanonical.x(), imgPosCanonical.y(), linear, textureIndex, &r, &g, &b, &a,&mmLevel);
                }
        }
    }
    if (!picked) {
        _imp->infoViewer[textureIndex]->setColorValid(false);
    } else {
        _imp->infoViewer[textureIndex]->setColorApproximated(mmLevel > 0);
        _imp->infoViewer[textureIndex]->setColorValid(true);
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

    if (!_imp->viewerTab) {
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
    if (modCASIsControl(e)) {
        const int delta_max = 28;
        // threshold delta to the range -delta_max..delta_max
        int delta = std::max(-delta_max, std::min(e->delta(), delta_max));
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
    
    boost::shared_ptr<NodeGuiI> nodeGui_i = _imp->viewerTab->getInternalNode()->getNode()->getNodeGui();
    boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(nodeGui_i);
    gui->selectNode(nodeGui);

    const double zoomFactor_min = 0.01;
    const double zoomFactor_max = 1024.;
    double zoomFactor;
    unsigned int oldMipMapLevel,newMipMapLevel;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
        zoomFactor = _imp->zoomCtx.factor() ;

        oldMipMapLevel = std::log(zoomFactor >= 1 ? 1 : std::pow( 2,-std::ceil(std::log(zoomFactor) / M_LN2) )) / M_LN2;

        zoomFactor*= scaleFactor;
        
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        
        newMipMapLevel = std::log(zoomFactor >= 1 ? 1 : std::pow( 2,-std::ceil(std::log(zoomFactor) / M_LN2) )) / M_LN2;
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
        _imp->zoomOrPannedSinceLastFit = true;
    }
    int zoomValue = (int)(100 * zoomFactor);
    if (zoomValue == 0) {
        zoomValue = 1; // sometimes, floor(100*0.01) makes 0
    }
    assert(zoomValue > 0);
    Q_EMIT zoomChanged(zoomValue);

    if (_imp->viewerTab->isViewersSynchroEnabled()) {
        _imp->viewerTab->synchronizeOtherViewersProjection();
    }

    _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
    

    ///Clear green cached line so the user doesn't expect to see things in the cache
    ///since we're changing the zoom factor
    if (oldMipMapLevel != newMipMapLevel) {
        _imp->viewerTab->clearTimelineCacheLine();
    }
    update();
}

void
ViewerGL::zoomSlot(double v)
{
    zoomSlot( (int)(std::floor(v+0.5)) );
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
    unsigned int oldMipMapLevel,newMipMapLevel;
    newMipMapLevel = std::log(newZoomFactor >= 1 ? 1 :
                              std::pow( 2,-std::ceil(std::log(newZoomFactor) / M_LN2) )) / M_LN2;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        oldMipMapLevel = std::log(_imp->zoomCtx.factor() >= 1 ? 1 :
                                  std::pow( 2,-std::ceil(std::log(_imp->zoomCtx.factor()) / M_LN2) )) / M_LN2;
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
    
    _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
   
}

void
ViewerGL::fitImageToFormat()
{
    fitImageToFormat(false);
}

void
ViewerGL::fitImageToFormat(bool useProjectFormat)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    // size in Canonical = Zoom coordinates !
    double w,h;
    const RectD& tex0RoD = _imp->currentViewerInfo[0].getRoD();
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
    unsigned int oldMipMapLevel,newMipMapLevel;
    {
        QMutexLocker(&_imp->zoomCtxMutex);
        old_zoomFactor = _imp->zoomCtx.factor();
        oldMipMapLevel = std::log(old_zoomFactor >= 1 ? 1 :
                                  std::pow( 2,-std::ceil(std::log(old_zoomFactor) / M_LN2) )) / M_LN2;

        // set the PAR first
        //_imp->zoomCtx.setZoom(0., 0., 1., 1.);
        // leave 4% of margin around
        _imp->zoomCtx.fit(-0.02 * w, 1.02 * w, -0.02 * h, 1.02 * h);
        zoomFactor = _imp->zoomCtx.factor();
        _imp->zoomOrPannedSinceLastFit = false;
    }
    newMipMapLevel = std::log(zoomFactor >= 1 ? 1 :
                              std::pow( 2,-std::ceil(std::log(zoomFactor) / M_LN2) )) / M_LN2;
    
    _imp->oldClick = QPoint(); // reset mouse posn

    if (old_zoomFactor != zoomFactor) {
        int zoomFactorInt = zoomFactor  * 100;
        if (zoomFactorInt == 0) {
            zoomFactorInt = 1;
        }
        Q_EMIT zoomChanged(zoomFactorInt);
    }
    
    if (_imp->viewerTab->isViewersSynchroEnabled()) {
        _imp->viewerTab->synchronizeOtherViewersProjection();
    }
    
    if (newMipMapLevel != oldMipMapLevel) {
        ///Clear green cached line so the user doesn't expect to see things in the cache
        ///since we're changing the zoom factor
        _imp->viewerTab->clearTimelineCacheLine();
    }
    
    _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
    
    update();
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
        setRegionOfDefinition(_imp->blankViewerInfo.getRoD(),_imp->blankViewerInfo.getDisplayWindow().getPixelAspectRatio(),0);
        setRegionOfDefinition(_imp->blankViewerInfo.getRoD(),_imp->blankViewerInfo.getDisplayWindow().getPixelAspectRatio(),1);
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

    return _imp->currentViewerInfo[textureIndex].getRoD();
}

/*The displayWindow of the currentFrame(Resolution)
   This is the same for both as we only use the display window as to indicate the project window.*/
Format
ViewerGL::getDisplayWindow() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->currentViewerInfo[0].getDisplayWindow();
}

void
ViewerGL::setRegionOfDefinition(const RectD & rod,
                                double par,
                                int textureIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->viewerTab->getGui()) {
        return;
    }
    
    RectI pixelRoD;
    rod.toPixelEnclosing(0, par, &pixelRoD);
    
    _imp->currentViewerInfo[textureIndex].setRoD(rod);
    if (_imp->infoViewer[textureIndex] && !_imp->viewerTab->getGui()->isGUIFrozen()) {
        _imp->infoViewer[textureIndex]->setDataWindow(pixelRoD);
    }
    

    QString left,btm,right,top;
    left.setNum(pixelRoD.left());
    btm.setNum(pixelRoD.bottom());
    right.setNum(pixelRoD.right());
    top.setNum(pixelRoD.top());


    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].append(left);
    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].append(",");
    _imp->currentViewerInfo_btmLeftBBOXoverlay[textureIndex].append(btm);
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].clear();
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].append(right);
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].append(",");
    _imp->currentViewerInfo_topRightBBOXoverlay[textureIndex].append(top);
}

void
ViewerGL::onProjectFormatChangedInternal(const Format & format,bool triggerRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if (!_imp->viewerTab->getGui()) {
        return;
    }
    RectD canonicalFormat = format.toCanonicalFormat();
    
    _imp->blankViewerInfo.setDisplayWindow(format);
    _imp->blankViewerInfo.setRoD(canonicalFormat);
    for (int i = 0; i < 2; ++i) {
        if (_imp->infoViewer[i]) {
            _imp->infoViewer[i]->setResolution(format);
            if (!_imp->activeTextures[i]) {
                _imp->currentViewerInfo[i].setRoD(canonicalFormat);
            }
        }
    }
    {
        QMutexLocker k(&_imp->projectFormatMutex);
        _imp->projectFormat = format;
    }
    _imp->currentViewerInfo_resolutionOverlay.clear();
    _imp->currentViewerInfo_resolutionOverlay.append( QString::number(format.width() ) );
    _imp->currentViewerInfo_resolutionOverlay.append("x");
    _imp->currentViewerInfo_resolutionOverlay.append( QString::number(format.height() ) );
    
    bool loadingProject = _imp->viewerTab->getGui()->getApp()->getProject()->isLoadingProject();
    if ( !loadingProject && triggerRender) {
        fitImageToFormat(true);
    }
    
    
    {
        double w3 = canonicalFormat.width() / 3.;
        double h3 = canonicalFormat.height() / 3.;
        RectD userRoi;
        userRoi.x1 = canonicalFormat.x1 + w3;
        userRoi.x2 = canonicalFormat.x2 - w3;
        userRoi.y1 = canonicalFormat.y1 + h3;
        userRoi.y2 = canonicalFormat.y2 - h3;
        QMutexLocker l(&_imp->userRoIMutex);
        _imp->userRoI = userRoi;
    }
    
    if (!loadingProject) {
        update();
    }

}

void
ViewerGL::onProjectFormatChanged(const Format & format)
{
    onProjectFormatChangedInternal(format, true);
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
        _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
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
    double scale = 1. / (1 << getCurrentRenderScale());
    if ( _imp->viewerTab->notifyOverlaysFocusGained(RenderScale(scale)) ) {
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

    double scale = 1. / (1 << getCurrentRenderScale());
    if ( _imp->viewerTab->notifyOverlaysFocusLost(RenderScale(scale)) ) {
        update();
    }
    QGLWidget::focusOutEvent(e);
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


ImageBitDepthEnum
ViewerGL::getBitDepth() const
{
    // MT-SAFE
    ///supportsGLSL is set on the main thread only once on startup, it doesn't need to be protected.
    if (!_imp->supportsGLSL) {
        return Natron::eImageBitDepthByte;
    } else {
        return appPTR->getCurrentSettings()->getViewersBitDepth();
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
    displayOverlaysAction->setChecked(_imp->overlay);
    QObject::connect( displayOverlaysAction,SIGNAL( triggered() ),this,SLOT( toggleOverlays() ) );
    
    
    QAction* toggleWipe = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDToggleWipe,kShortcutDescToggleWipe, _imp->menu);
    toggleWipe->setCheckable(true);
    toggleWipe->setChecked(getViewerTab()->getCompositingOperator() != Natron::eViewerCompositingOperatorNone);
    QObject::connect( toggleWipe,SIGNAL( triggered() ),this,SLOT( toggleWipe() ) );
    _imp->menu->addAction(toggleWipe);
    
    
    QAction* centerWipe = new ActionWithShortcut(kShortcutGroupViewer,kShortcutIDCenterWipe,kShortcutDescCenterWipe, _imp->menu);
    QObject::connect( centerWipe,SIGNAL( triggered() ),this,SLOT( centerWipe() ) );
    _imp->menu->addAction(centerWipe);
    
    QAction* goToPrevLayer = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDPrevLayer, kShortcutDescPrevLayer, _imp->menu);
    QObject::connect( goToPrevLayer,SIGNAL( triggered() ),_imp->viewerTab,SLOT( previousLayer() ) );
    _imp->menu->addAction(goToPrevLayer);
    QAction* goToNextLayer = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDNextLayer, kShortcutDescNextLayer, _imp->menu);
    QObject::connect( goToNextLayer,SIGNAL( triggered() ),_imp->viewerTab,SLOT( nextLayer() ) );
    _imp->menu->addAction(goToNextLayer);
    
    QAction* switchAB = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDSwitchInputAAndB, kShortcutDescSwitchInputAAndB, _imp->menu);
    QObject::connect( switchAB,SIGNAL( triggered() ),_imp->viewerTab,SLOT( switchInputAAndB() ) );
    _imp->menu->addAction(switchAB);
    
    Natron::Menu* showHideMenu = new Natron::Menu(tr("Show/Hide"),_imp->menu);
    //showHideMenu->setFont(QFont(appFont,appFontSize));
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
    if (w <= 0 || h <= 0 || right <= left || top <= bottom) {
        return;
    }
    double scalex = (right-left) / w;
    double scaley = (top-bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font);
    glCheckError();
}

void
ViewerGL::updatePersistentMessageToWidth(int w)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if (!_imp->viewerTab || !_imp->viewerTab->getGui()) {
        return;
    }
    
    const std::list<DockablePanel*>& openedPanels = _imp->viewerTab->getGui()->getVisiblePanels();
    
    _imp->persistentMessages.clear();
    QStringList allMessages;
    
    int type = 0;
    ///Draw overlays in reverse order of appearance
    std::list<DockablePanel*>::const_iterator next = openedPanels.begin();
    if (next != openedPanels.end()) {
        ++next;
    }
    int nbNonEmpty = 0;
    for (std::list<DockablePanel*>::const_iterator it = openedPanels.begin(); it != openedPanels.end(); ++it) {
        
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
        if (!mess.isEmpty()) {
             allMessages.append(mess);
            ++nbNonEmpty;
        }
        if (next != openedPanels.end()) {
            ++next;
        }
        
        if (!mess.isEmpty()) {
            type = (nbNonEmpty == 1 && nType == 2) ? 2 : 1;
        }
    }
    _imp->persistentMessageType = type;
    
    QFontMetrics fm(*_imp->textFont);
    
    for (int i = 0; i < allMessages.size(); ++i) {
        QStringList wordWrapped = wordWrap(fm,allMessages[i], w - PERSISTENT_MESSAGE_LEFT_OFFSET_PIXELS);
        for (int j = 0; j < wordWrapped.size(); ++j) {
            _imp->persistentMessages.push_back(wordWrapped[j]);
        }
    }
    
    _imp->displayPersistentMessage = !_imp->persistentMessages.isEmpty();
    update();
}

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

bool
ViewerGL::isVisibleInViewport(const RectD& rectangle) const
{
    RectD bbox;
    {
        QMutexLocker l(&_imp->zoomCtxMutex);
        bbox.x1 = _imp->zoomCtx.left();
        bbox.y1 = _imp->zoomCtx.bottom();
        bbox.x2 = _imp->zoomCtx.right();
        bbox.y2 = _imp->zoomCtx.top();
    }
    return bbox.intersects(rectangle);
}

void
ViewerGL::setBuildNewUserRoI(bool b)
{
    _imp->buildUserRoIOnNextPress = b;
    
    QMutexLocker k(&_imp->userRoIMutex);
    _imp->draggedUserRoI = _imp->userRoI;
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
    if (!b) {
        _imp->buildUserRoIOnNextPress = false;
    }
    if ( displayingImage() ) {
        _imp->viewerTab->getInternalNode()->renderCurrentFrame(false);
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
        _imp->viewerTab->discardInternalNodePointer();
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

bool
ViewerGL::pickColorInternal(double x, double y)
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
        unsigned int mmLevel;
        bool picked = getColorAt(imgPos.x(), imgPos.y(), linear, i, &r, &g, &b, &a,&mmLevel);
        if (picked) {
            if (i == 0) {
                _imp->viewerTab->getGui()->setColorPickersColor(r,g,b,a);
            }
            _imp->infoViewer[i]->setColorApproximated(mmLevel > 0);
            _imp->infoViewer[i]->setColorValid(true);
            if ( !_imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->showColorAndMouseInfo();
            }
            _imp->infoViewer[i]->setColor(r,g,b,a);
            ret = true;
        } else {
            _imp->infoViewer[i]->setColorValid(false);
        }
    }
    
    return ret;
}

// used for the ctrl-click color picker (not the information bar at the bottom of the viewer)
bool
ViewerGL::pickColor(double x,
                    double y)
{
    bool isSync = _imp->viewerTab->isViewersSynchroEnabled();
    if (isSync) {
        bool res = false;
        const std::list<ViewerTab*>& allViewers = _imp->viewerTab->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = allViewers.begin(); it!=allViewers.end(); ++it) {
            bool ret = (*it)->getViewer()->pickColorInternal(x, y);
            if ((*it)->getViewer() == this) {
                res = ret;
            }
        }
        return res;
    } else {
        return pickColorInternal(x, y);
    }
}


void
ViewerGL::updateInfoWidgetColorPicker(const QPointF & imgPos,
                                 const QPoint & widgetPos)
{
    Format dispW = getDisplayWindow();
    RectD canonicalDispW = dispW.toCanonicalFormat();
    
    NodePtr rotoPaintNode;
    boost::shared_ptr<RotoStrokeItem> curStroke;
    bool isDrawing;
    _imp->viewerTab->getGui()->getApp()->getActiveRotoDrawingStroke(&rotoPaintNode, &curStroke, &isDrawing);
    
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
    if (!_imp->viewerTab || !_imp->viewerTab->getGui() || _imp->viewerTab->getGui()->isGUIFrozen()) {
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
            if (_imp->pickerState == ePickerStateInactive) {
                //if ( !_imp->viewerTab->getInternalNode()->getRenderEngine()->hasThreadsWorking() ) {
                    updateColorPicker( texIndex,widgetPos.x(),widgetPos.y() );
               // }
            } else if ( ( _imp->pickerState == ePickerStatePoint) || ( _imp->pickerState == ePickerStateRectangle) ) {
                if ( !_imp->infoViewer[texIndex]->colorAndMouseVisible() ) {
                    _imp->infoViewer[texIndex]->showColorAndMouseInfo();
                }
            } else {
                ///unkwn state
                assert(false);
            }
            double par = _imp->currentViewerInfo[texIndex].getDisplayWindow().getPixelAspectRatio();
            QPoint imgPosPixel;
            imgPosPixel.rx() = std::floor(imgPos.x() / par);
            imgPosPixel.ry() = std::floor(imgPos.y());
            _imp->infoViewer[texIndex]->setMousePos(imgPosPixel);
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
    bool isSync = _imp->viewerTab->isViewersSynchroEnabled();
    if (isSync) {
        const std::list<ViewerTab*>& allViewers = _imp->viewerTab->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = allViewers.begin(); it!=allViewers.end(); ++it) {
            (*it)->getViewer()->updateRectangleColorPickerInternal();
        }
    } else {
        updateRectangleColorPickerInternal();
    }
}

void
ViewerGL::updateRectangleColorPickerInternal()
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
        unsigned int mm;
        bool picked = getColorAtRect(rect, linear, i, &r, &g, &b, &a,&mm);
        if (picked) {
            if (i == 0) {
                _imp->viewerTab->getGui()->setColorPickersColor(r,g,b,a);
            }
            _imp->infoViewer[i]->setColorValid(true);
            if ( !_imp->infoViewer[i]->colorAndMouseVisible() ) {
                _imp->infoViewer[i]->showColorAndMouseInfo();
            }
            _imp->infoViewer[i]->setColorApproximated(mm > 0);
            _imp->infoViewer[i]->setColor(r, g, b, a);
        } else {
            _imp->infoViewer[i]->setColorValid(false);
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
        _imp->getProjectFormatCanonical(rod);
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
ViewerGL::isWipeHandleVisible() const
{
    return _imp->viewerTab->getCompositingOperator() != eViewerCompositingOperatorNone;
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

ViewerCompositingOperatorEnum
ViewerGL::getCompositingOperator() const
{
    return _imp->viewerTab->getCompositingOperator();
}

void
ViewerGL::setCompositingOperator(ViewerCompositingOperatorEnum op)
{
    _imp->viewerTab->setCompositingOperator(op);
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

    if ( (type == Texture::eDataTypeByte) || !_imp->supportsGLSL ) {
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
    } else if ( (type == Texture::eDataTypeFloat) && _imp->supportsGLSL ) {
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
    glCheckAttribStack();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack();
    glPushClientAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glCheckProjectionStack();
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glCheckModelviewStack();
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
            for (U32 j = 0; j < _imp->lastRenderedTiles[i].size(); ++j) {
                _imp->lastRenderedTiles[i][j].clear();
            }
            toUnRegister += _imp->memoryHeldByLastRenderedImages[i];
        }
        if (toUnRegister > 0) {
            getInternalNode()->unregisterPluginMemory(toUnRegister);
        }
        
    }
}


void
ViewerGL::getLastRenderedImage(int textureIndex, std::list<boost::shared_ptr<Natron::Image> >* ret) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if ( !getInternalNode()->getNode()->isActivated() ) {
        return;
    }
    QMutexLocker l(&_imp->lastRenderedImageMutex);
    for (U32 i = 0; i < _imp->lastRenderedTiles[textureIndex].size(); ++i) {
        if (!_imp->lastRenderedTiles[textureIndex][i].empty()) {
            *ret = _imp->lastRenderedTiles[textureIndex][i];
        }
    }

}

void
ViewerGL::getLastRenderedImageByMipMapLevel(int textureIndex,unsigned int mipMapLevel, std::list<boost::shared_ptr<Natron::Image> >* ret) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if ( !getInternalNode()->getNode()->isActivated() ) {
        return ;
    }
    
    QMutexLocker l(&_imp->lastRenderedImageMutex);
    assert(_imp->lastRenderedTiles[textureIndex].size() > mipMapLevel);
    
    if (!_imp->lastRenderedTiles[textureIndex][mipMapLevel].empty()) {
        *ret = _imp->lastRenderedTiles[textureIndex][mipMapLevel];
    }
    
    if (!ret->empty()) {
        return;
    }
    
    //Find an image at higher scale
    if (mipMapLevel > 0) {
        for (int i = (int)mipMapLevel - 1; i >= 0; --i) {
            if (!_imp->lastRenderedTiles[textureIndex][i].empty()) {
                *ret =  _imp->lastRenderedTiles[textureIndex][i];
            }
        }
    }
    
    if (!ret->empty()) {
        return;
    }
    
    //Find an image at lower scale
    for (U32 i = mipMapLevel + 1; i < _imp->lastRenderedTiles[textureIndex].size(); ++i) {
        if (!_imp->lastRenderedTiles[textureIndex][i].empty()) {
            *ret = _imp->lastRenderedTiles[textureIndex][i];
        }
    }
}

#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif
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


unsigned int
ViewerGL::getCurrentRenderScale() const
{
    return getMipMapLevelCombinedToZoomFactor();
}

template <typename PIX,int maxValue>
static
bool
getColorAtInternal(const ImageList& tiles,
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
    for (ImageList::const_iterator it = tiles.begin(); it!=tiles.end(); ++it) {
        if ((*it)->getBounds().contains(x,y)) {
            Image::ReadAccess racc(it->get());
            const PIX* pix = (const PIX*)racc.pixelAt(x, y);
            
            if (!pix) {
                return false;
            }
            
            int  nComps = (*it)->getComponents().getNumComponents();
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
    ImageList tiles;
    getLastRenderedImageByMipMapLevel(textureIndex,mipMapLevel,&tiles);

    
    if (tiles.empty()) {
        return false;
        ///Don't do this as this is 8bit data
        /*double colorGPU[4];
        getTextureColorAt(x, y, &colorGPU[0], &colorGPU[1], &colorGPU[2], &colorGPU[3]);
        *a = colorGPU[3];
        if ( forceLinear && (_imp->displayingImageLut != eViewerColorSpaceLinear) ) {
            const Natron::Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
            
            *r = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[0]);
            *g = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[1]);
            *b = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[2]);
        } else {
            *r = colorGPU[0];
            *g = colorGPU[1];
            *b = colorGPU[2];
        }
        return true;*/
    }
    
    const ImagePtr& firstTile = tiles.front();
    
    ImageBitDepthEnum depth = firstTile->getBitDepth();
    ViewerColorSpaceEnum srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Natron::Color::Lut* dstColorSpace;
    const Natron::Color::Lut* srcColorSpace;
    if ( (srcCS == _imp->displayingImageLut)
        && ( (_imp->displayingImageLut == eViewerColorSpaceLinear) || !forceLinear ) ) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        if (firstTile->getComponents().isColorPlane()) {
            srcColorSpace = ViewerInstance::lutFromColorspace(srcCS);
            dstColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
        } else {
            srcColorSpace = dstColorSpace = 0;
        }
    }
    
    const double par = firstTile->getPixelAspectRatio();
    
    double scale = 1. / (1 << firstTile->getMipMapLevel());
    
    ///Convert to pixel coords
    int xPixel = std::floor(x  * scale / par);
    int yPixel = std::floor(y * scale);
    bool gotval;
    switch (depth) {
        case eImageBitDepthByte:
            gotval = getColorAtInternal<unsigned char, 255>(tiles,
                                                            xPixel, yPixel,
                                                            forceLinear,
                                                            srcColorSpace,
                                                            dstColorSpace,
                                                            r, g, b, a);
            break;
        case eImageBitDepthShort:
            gotval = getColorAtInternal<unsigned short, 65535>(tiles,
                                                               xPixel, yPixel,
                                                               forceLinear,
                                                               srcColorSpace,
                                                               dstColorSpace,
                                                               r, g, b, a);
            break;
        case eImageBitDepthFloat:
            gotval = getColorAtInternal<float, 1>(tiles,
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
    *imgMmlevel = firstTile->getMipMapLevel();
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

    ImageList tiles;
    getLastRenderedImageByMipMapLevel(textureIndex, mipMapLevel, &tiles);
  
    if (!tiles.empty()) {
        mipMapLevel = tiles.front()->getMipMapLevel();
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
    if ( tiles.empty() ) {
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

        if ( (type == Texture::eDataTypeByte) || !_imp->supportsGLSL ) {
            std::vector<U32> pixels(rectPixel.width() * rectPixel.height());
            glReadBuffer(GL_FRONT);
            glReadPixels(rectPixel.left(), rectPixel.right(), rectPixel.width(), rectPixel.height(),
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
                    const Natron::Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
                    
                    rSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(rF);
                    gSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(gF);
                    bSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(bF);
                } else {
                    rSum += rF;
                    gSum += gF;
                    bSum += bF;
                }

            }
            
            glCheckError();
        } else if ( (type == Texture::eDataTypeFloat) && _imp->supportsGLSL ) {
            std::vector<float> pixels(rectPixel.width() * rectPixel.height() * 4);
            glReadPixels(rectPixel.left(), rectPixel.right(), rectPixel.width(), rectPixel.height(),
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
                        const Natron::Color::Lut* srcColorSpace = ViewerInstance::lutFromColorspace(_imp->displayingImageLut);
                        
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
          

            glCheckError();
        }
 
        *r = rSum / rectPixel.area();
        *g = gSum / rectPixel.area();
        *b = bSum / rectPixel.area();
        *a = aSum / rectPixel.area();
        
        return true;*/
    }
    
    
    ImageBitDepthEnum depth = tiles.front()->getBitDepth();
    ViewerColorSpaceEnum srcCS = _imp->viewerTab->getGui()->getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Natron::Color::Lut* dstColorSpace;
    const Natron::Color::Lut* srcColorSpace;
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
                    gotval = getColorAtInternal<unsigned char, 255>(tiles,
                                                                    xPixel, yPixel,
                                                                    forceLinear,
                                                                    srcColorSpace,
                                                                    dstColorSpace,
                                                                    &rPix, &gPix, &bPix, &aPix);
                    break;
                case eImageBitDepthShort:
                    gotval = getColorAtInternal<unsigned short, 65535>(tiles,
                                                                       xPixel, yPixel,
                                                                       forceLinear,
                                                                       srcColorSpace,
                                                                       dstColorSpace,
                                                                       &rPix, &gPix, &bPix, &aPix);
                    break;
                case eImageBitDepthHalf:
                    break;
                case eImageBitDepthFloat:
                    gotval = getColorAtInternal<float, 1>(tiles,
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
    if (_imp->activeTextures[0]) {
        return _imp->displayingImageTime[0];
    } else {
        return _imp->viewerTab->getTimeLine()->currentFrame();
    }
}

void
ViewerGL::getViewerFrameRange(int* first,int* last) const
{
    _imp->viewerTab->getTimelineBounds(first, last);
}

double
ViewerGL::currentTimeForEvent(QInputEvent* e)
{
#if QT_VERSION >= 0x050000
    // timestamp() is usually in milliseconds
    if (e->timestamp()) {
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



#include "moc_ViewerGL.cpp"

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

#include "AnimationModuleView.h"

#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
#include <QApplication>

#include <QtCore/QThread>
#include <QImage>

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/OSGLFunctions.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleViewPrivate.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/Menu.h"
#include "Gui/NodeAnim.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/PythonPanels.h"
#include "Gui/TableItemAnim.h"
#include "Gui/TabWidget.h"

// in pixels
#define CLICK_DISTANCE_TOLERANCE 5
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8
#define BOUNDING_BOX_HANDLE_SIZE 4


#define CURVE_EDITOR_MIN_HEIGHT_PX 50


NATRON_NAMESPACE_ENTER



AnimationModuleView::AnimationModuleView(QWidget* parent)
: QGLWidget(parent)
, _imp()
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
}

AnimationModuleView::~AnimationModuleView()
{

}

void
AnimationModuleView::initialize(Gui* gui, const AnimationModuleBasePtr& model)
{
    if (_imp) {
        return;
    }
    _imp.reset(new AnimationModuleViewPrivate(gui, this, model));

    connect( model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), this, SLOT(onSelectionModelKeyframeSelectionChanged()) );


    TimeLinePtr timeline = model->getTimeline();
    AnimationModulePtr isAnimModule = toAnimationModule(model);
    if (timeline) {
        ProjectPtr project = gui->getApp()->getProject();
        assert(project);
        QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimeLineFrameChanged(SequenceTime,int)) );
        QObject::connect( project.get(), SIGNAL(frameRangeChanged(int,int)), this, SLOT(update()) );
        onTimeLineFrameChanged(timeline->currentFrame(), eValueChangedReasonUserEdited);
    }
    if (isAnimModule) {
        _imp->treeView = isAnimModule->getEditor()->getTreeView();
        _imp->sizeH = QSize(10000,10000);
    } else {
        _imp->sizeH = QSize(400, 400);
    }
}


QSize
AnimationModuleView::sizeHint() const
{
    return _imp->sizeH;
}



RectD
AnimationModuleView::getSelectionRectangle() const
{
    return _imp->selectionRect;
}

void
AnimationModuleView::initializeGL()
{
    appPTR->initializeOpenGLFunctionsOnce();

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    _imp->generateKeyframeTextures();
}

void
AnimationModuleView::resizeGL(int width,
                      int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
    GL_GPU::Viewport (0, 0, width, height);

    //int treeItemBottomWidgetCoordY = _imp->treeView ? _imp->treeView->getTreeBottomYWidgetCoords() : 0;
    
    // Width and height may be 0 when tearing off a tab to another panel
    if ( (width > 0) && (height > 0) ) {
        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->curveEditorZoomContext.setScreenSize(width, height);
        _imp->dopeSheetZoomContext.setScreenSize(width, height);
    }

    if (height == 1) {
        //don't do the following when the height of the widget is irrelevant
        return;
    }

    if (!_imp->zoomOrPannedSinceLastFit) {
        centerOnAllItems();
    }
}


void
AnimationModuleView::paintGL()
{

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError(GL_GPU);
    if (_imp->curveEditorZoomContext.factor() <= 0) {
        return;
    }

    _imp->drawnOnce = true;

    double bgR, bgG, bgB;
    getBackgroundColour(bgR, bgG, bgB);

    GL_GPU::ClearColor(bgR, bgG, bgB, 1.);
    GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
    glCheckErrorIgnoreOSXBug(GL_GPU);


    if ( (_imp->curveEditorZoomContext.left() == _imp->curveEditorZoomContext.right()) || (_imp->curveEditorZoomContext.bottom() == _imp->curveEditorZoomContext.top()) ) {
        return;
    }


    AnimationModuleEditor::AnimationModuleDisplayViewMode displayMode = AnimationModuleEditor::eAnimationModuleDisplayViewModeCurveEditor;
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (isAnimModule) {
        displayMode = isAnimModule->getEditor()->getDisplayMode();
    }

    int w = width();
    int h = height();
    int treeItemBottomWidgetCoordY = 0;
    bool drawCurveEditor = true;

    if (displayMode == AnimationModuleEditor::eAnimationModuleDisplayViewModeStacked) {

        treeItemBottomWidgetCoordY = _imp->treeView ? _imp->treeView->getTreeBottomYWidgetCoords() : 0;
        drawCurveEditor = treeItemBottomWidgetCoordY < h - CURVE_EDITOR_MIN_HEIGHT_PX;

        assert(isAnimModule);
        if (treeItemBottomWidgetCoordY <=  0) {
            treeItemBottomWidgetCoordY = 0;
            displayMode = AnimationModuleEditor::eAnimationModuleDisplayViewModeCurveEditor;
        } else {

            GL_GPU::MatrixMode(GL_PROJECTION);
            GL_GPU::LoadIdentity();
            GL_GPU::Ortho(_imp->dopeSheetZoomContext.left(), _imp->dopeSheetZoomContext.right(), _imp->dopeSheetZoomContext.bottom(), _imp->dopeSheetZoomContext.top(), 1, -1);
            GL_GPU::MatrixMode(GL_MODELVIEW);
            GL_GPU::LoadIdentity();
            glCheckError(GL_GPU);
            
            
            GL_GPU::Enable(GL_SCISSOR_TEST);
            int scissorY = drawCurveEditor ? h - treeItemBottomWidgetCoordY - 1 : 0;
            int scissorH = drawCurveEditor ? treeItemBottomWidgetCoordY + 1 : h;
            GL_GPU::Scissor(0, scissorY, w, scissorH);
            glCheckError(GL_GPU);
            
            _imp->drawDopeSheetView();
            glCheckError(GL_GPU);
        }
        
    }

    GL_GPU::MatrixMode(GL_PROJECTION);
    GL_GPU::LoadIdentity();
    GL_GPU::Ortho(_imp->curveEditorZoomContext.left(), _imp->curveEditorZoomContext.right(), _imp->curveEditorZoomContext.bottom(), _imp->curveEditorZoomContext.top(), 1, -1);
    GL_GPU::MatrixMode(GL_MODELVIEW);
    GL_GPU::LoadIdentity();
    glCheckError(GL_GPU);


    if (drawCurveEditor) {

        if (displayMode == AnimationModuleEditor::eAnimationModuleDisplayViewModeStacked) {
            GL_GPU::Scissor(0, 0, w, h - treeItemBottomWidgetCoordY);
        }
        _imp->drawCurveEditorView();
        glCheckError(GL_GPU);
    }
    GL_GPU::Disable(GL_SCISSOR_TEST);

    if (displayMode == AnimationModuleEditor::eAnimationModuleDisplayViewModeStacked && drawCurveEditor) {

        // Draw a line to split dope sheet and curve editor
        QPointF bottomLeft = _imp->curveEditorZoomContext.toZoomCoordinates(0, 0);
        QPointF topRight = _imp->curveEditorZoomContext.toZoomCoordinates(w, h);
        
        double treeItemBottomZoomCord = _imp->curveEditorZoomContext.toZoomCoordinates(0, treeItemBottomWidgetCoordY).y();
        if (treeItemBottomWidgetCoordY < h) {
            glCheckError(GL_GPU);
            GL_GPU::LineWidth(1.5);
            GL_GPU::Color3d(0, 0, 0);
            GL_GPU::Begin(GL_LINES);
            assert(treeItemBottomZoomCord == treeItemBottomZoomCord);
            assert(topRight.x() == topRight.x());
            assert(bottomLeft.x() == bottomLeft.x());
            GL_GPU::Vertex2d(bottomLeft.x(), treeItemBottomZoomCord);
            GL_GPU::Vertex2d(topRight.x(), treeItemBottomZoomCord);
            GL_GPU::End();
            GL_GPU::LineWidth(1.);
            glCheckError(GL_GPU);
        }
    }
    
    _imp->drawTimelineMarkers(_imp->curveEditorZoomContext);

    glCheckError(GL_GPU);
} // paintGL



void
AnimationModuleView::getBackgroundColour(double &r,
                                         double &g,
                                         double &b) const
{
    // use the same settings as the curve editor
    appPTR->getCurrentSettings()->getAnimationModuleEditorBackgroundColor(&r, &g, &b);
}


void
AnimationModuleView::swapOpenGLBuffers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    swapBuffers();
}


void
AnimationModuleView::redraw()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    update();
}

bool
AnimationModuleView::hasDrawnOnce() const
{
    return _imp->drawnOnce;
}

void
AnimationModuleView::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
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


void
AnimationModuleView::getViewportSize(double &width, double &height) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    width = this->width();
    height = this->height();
}

void
AnimationModuleView::getPixelScale(double & xScale, double & yScale) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // Use curve editor zoom context for the overlay callbacks, the dopesheet does not
    // have overlays.
    xScale = _imp->curveEditorZoomContext.screenPixelWidth();
    yScale = _imp->curveEditorZoomContext.screenPixelHeight();
}

#ifdef OFX_EXTENSIONS_NATRON
double
AnimationModuleView::getScreenPixelRatio() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return windowHandle()->devicePixelRatio()
#else
    return 1.;
#endif
}
#endif

RectD
AnimationModuleView::getViewportRect() const
{
    RectD bbox;
    {
        // Use curve editor zoom context for the overlay callbacks, the dopesheet does not
        // have overlays.
        bbox.x1 = _imp->curveEditorZoomContext.left();
        bbox.y1 = _imp->curveEditorZoomContext.bottom();
        bbox.x2 = _imp->curveEditorZoomContext.right();
        bbox.y2 = _imp->curveEditorZoomContext.top();
    }

    return bbox;
}

void
AnimationModuleView::getCursorPosition(double& x,
                               double& y) const
{
    QPoint p = QCursor::pos();

    // Use curve editor zoom context for the overlay callbacks, the dopesheet does not
    // have overlays.
    p = mapFromGlobal(p);
    QPointF mappedPos = _imp->curveEditorZoomContext.toZoomCoordinates( p.x(), p.y() );
    x = mappedPos.x();
    y = mappedPos.y();
}

void
AnimationModuleView::saveOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack(GL_GPU);

    // Don't protect GL_COLOR_BUFFER_BIT, because it seems to hit an OpenGL bug on
    // some macOS configurations (10.10-10.12), where garbage is displayed in the viewport.
    // see https://github.com/MrKepzie/Natron/issues/1460

   // GL_GPU::PushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack(GL_GPU);
    GL_GPU::PushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
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

void
AnimationModuleView::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    GL_GPU::BindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    GL_GPU::MatrixMode(GL_PROJECTION);
    GL_GPU::PopMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    GL_GPU::PopMatrix();
    GL_GPU::PopClientAttrib();
   // GL_GPU::PopAttrib();
}

bool
AnimationModuleView::renderText(double x,
                              double y,
                              const std::string &string,
                              double r,
                              double g,
                              double b,
                              double a,
                              int flags)
{
    QColor c;

    // Use curve editor zoom context for the overlay callbacks, the dopesheet does not
    // have overlays.
    c.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    c.setAlphaF(Image::clamp(a, 0., 1.));
    _imp->renderText(_imp->curveEditorZoomContext,  x, y, QString::fromUtf8( string.c_str() ), c, font(), flags );
    
    return true;
}



void
AnimationModuleView::toWidgetCoordinates(double *x, double *y) const
{
    // Use curve editor zoom context for the overlay callbacks, the dopesheet does not
    // have overlays.
    QPointF p = _imp->curveEditorZoomContext.toWidgetCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

void
AnimationModuleView::toCanonicalCoordinates(double *x, double *y) const
{
    // Use curve editor zoom context for the overlay callbacks, the dopesheet does not
    // have overlays.
    QPointF p = _imp->curveEditorZoomContext.toZoomCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

int
AnimationModuleView::getWidgetFontHeight() const
{
    return fontMetrics().height();
}

int
AnimationModuleView::getStringWidthForCurrentFont(const std::string& string) const
{
    return fontMetrics().width( QString::fromUtf8( string.c_str() ) );
}


#if 0
void
AnimationModuleView::getProjection(double *zoomLeft,
                           double *zoomBottom,
                           double *zoomFactor,
                           double *zoomAspectRatio) const
{
    QMutexLocker k(&_imp->zoomCtxMutex);

    *zoomLeft = _imp->curveEditorZoomContext.left();
    *zoomBottom = _imp->curveEditorZoomContext.bottom();
    *zoomFactor = _imp->curveEditorZoomContext.factor();
    *zoomAspectRatio = _imp->curveEditorZoomContext.aspectRatio();
}

void
AnimationModuleView::setProjection(double zoomLeft,
                           double zoomBottom,
                           double zoomFactor,
                           double zoomAspectRatio)
{
    // always running in the main thread
    QMutexLocker k(&_imp->zoomCtxMutex);

    _imp->curveEditorZoomContext.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
    double zoomRight = zoomLeft + _imp->dopeSheetZoomContext.screenWidth() * 1. / (zoomFactor * zoomAspectRatio);
    double dopeSheetBtm = _imp->dopeSheetZoomContext.bottom();
    double dopeSheetTop = _imp->dopeSheetZoomContext.top();
    if (zoomLeft < zoomRight && dopeSheetBtm < dopeSheetTop) {
        _imp->dopeSheetZoomContext.fill(zoomLeft, zoomRight, _imp->dopeSheetZoomContext.bottom(), _imp->dopeSheetZoomContext.top());
    }
}
#endif

void
AnimationModuleView::onTimeLineFrameChanged(SequenceTime,
                                    int /*reason*/)
{
    if ( !_imp->_gui || _imp->_gui->isGUIFrozen() ) {
        return;
    }

    if ( isVisible() ) {
        update();
    }
}


void
AnimationModuleView::onRemoveSelectedKeyFramesActionTriggered()
{
    _imp->_model.lock()->deleteSelectedKeyframes();
}

void
AnimationModuleView::onCopySelectedKeyFramesToClipBoardActionTriggered()
{
    _imp->_model.lock()->copySelectedKeys();
}

void
AnimationModuleView::onPasteClipBoardKeyFramesActionTriggered()
{
    AnimationModuleBasePtr model = _imp->_model.lock();
    model->pasteKeys(model->getKeyFramesClipBoard(), true /*relative*/);
}

void
AnimationModuleView::onPasteClipBoardKeyFramesAbsoluteActionTriggered()
{
    AnimationModuleBasePtr model = _imp->_model.lock();
    model->pasteKeys(model->getKeyFramesClipBoard(), false /*relative*/);
}

void
AnimationModuleView::onSelectAllKeyFramesActionTriggered()
{
    _imp->_model.lock()->getSelectionModel()->selectAllVisibleCurvesKeyFrames();
}

void
AnimationModuleView::onSetInterpolationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    KeyframeTypeEnum type = (KeyframeTypeEnum)action->data().toInt();
    _imp->_model.lock()->setSelectedKeysInterpolation(type);
}

void
AnimationModuleView::onCenterAllCurvesActionTriggered()
{
    centerOnAllItems();
    
}

void
AnimationModuleView::onCenterOnSelectedCurvesActionTriggered()
{
    centerOnSelection();
}

void
AnimationModuleView::centerOnItemsInternal(const std::vector<CurveGuiPtr>& curves,
                                           const std::vector<NodeAnimPtr>& nodes,
                                           const std::vector<TableItemAnimPtr>& tableItems)
{
    RectD ret;


    // First get the bbox of all curves keyframes
    // If we don't have a display range for curves, then
    // use the keyframes bbox that we pad of a few pixels
    bool gotCurvesWithDisplayRange = false;
    ret = getCurvesDisplayRangesBbox(curves);
    if (ret.isNull()) {
        ret = getCurvesKeyframeBbox(curves);
    } else {
        gotCurvesWithDisplayRange = true;
    }

    // Then merge nodes ranges
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->isRangeDrawingEnabled()) {
            RangeD frameRange = nodes[i]->getFrameRange();
            if (ret.isNull()) {
                ret.x1 = frameRange.min;
                ret.x2 = frameRange.max;
            } else {
                ret.x1 = std::min(ret.x1, frameRange.min);
                ret.x2 = std::max(ret.x2, frameRange.max);
            }
        }
    }

    // Then merge table items ranges
    for (std::size_t i = 0; i < tableItems.size(); ++i) {
        if (tableItems[i]->isRangeDrawingEnabled()) {
            RangeD frameRange = tableItems[i]->getFrameRange();
            if (ret.isNull()) {
                ret.x1 = frameRange.min;
                ret.x2 = frameRange.max;
            } else {
                ret.x1 = std::min(ret.x1, frameRange.min);
                ret.x2 = std::max(ret.x2, frameRange.max);
            }
        }
    }

    if (ret.y2 > ret.y1) {
        if (!gotCurvesWithDisplayRange) {
            ret.addPaddingPercentage(0.1, 0.1);
        }
        centerOn(ret.x1, ret.x2, ret.y1, ret.y2);
    } else {
        centerOn(ret.x1, ret.x2);
    }

}

void
AnimationModuleView::centerOnAllItems()
{
    AnimationModuleBasePtr model = _imp->_model.lock();

    std::vector<CurveGuiPtr> allVisibleCurves;
    std::vector<NodeAnimPtr> allNodes;
    std::vector<TableItemAnimPtr> allTableItems;

    model->getSelectionModel()->getAllItems(false, 0, &allNodes, &allTableItems);
    allVisibleCurves = _imp->getVisibleCurves();

    centerOnItemsInternal(allVisibleCurves, allNodes, allTableItems);

}

void
AnimationModuleView::centerOnSelection()
{
    AnimationModuleBasePtr model = _imp->_model.lock();
    std::vector<CurveGuiPtr> selectedCurves =  _imp->getSelectedCurves();
    const std::list<NodeAnimPtr>& selectedNodes = model->getSelectionModel()->getCurrentNodesSelection();
    const std::list<TableItemAnimPtr>& selectedTableItems = model->getSelectionModel()->getCurrentTableItemsSelection();

    if ( selectedCurves.empty() && selectedNodes.empty() && selectedTableItems.empty() ) {
        centerOnAllItems();
    } else {
        std::vector<NodeAnimPtr> nodes;
        nodes.insert(nodes.end(), selectedNodes.begin(), selectedNodes.end());
        std::vector<TableItemAnimPtr> tableItems;
        tableItems.insert(tableItems.end(), selectedTableItems.begin(), selectedTableItems.end());
        centerOnItemsInternal(selectedCurves, nodes, tableItems);
    }
}

void
AnimationModuleView::centerOn(double xmin,
                              double xmax,
                              double ymin,
                              double ymax)

{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        QMutexLocker k(&_imp->zoomCtxMutex);

        if ( (_imp->dopeSheetZoomContext.screenWidth() > 0) && (_imp->dopeSheetZoomContext.screenHeight() > 0) ) {
            if (xmax > xmin) {
                _imp->dopeSheetZoomContext.fill(xmin, xmax, _imp->dopeSheetZoomContext.bottom(), _imp->dopeSheetZoomContext.top() );
            }
        }
        if ( (_imp->curveEditorZoomContext.screenWidth() > 0) && (_imp->curveEditorZoomContext.screenHeight() > 0) ) {
            if (xmax > xmin && ymax > ymin) {
                _imp->curveEditorZoomContext.fill(xmin, xmax, ymin, ymax);
            }
        }
        _imp->zoomOrPannedSinceLastFit = false;
    }
    update();
}

void
AnimationModuleView::centerOn(double xmin,  double xmax)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (xmax <= xmin) {
        return;
    }
    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        if ( (_imp->dopeSheetZoomContext.screenWidth() > 0) && (_imp->dopeSheetZoomContext.screenHeight() > 0) ) {

            _imp->dopeSheetZoomContext.fill( xmin, xmax, _imp->dopeSheetZoomContext.bottom(), _imp->dopeSheetZoomContext.top() );
        }
        if ( (_imp->curveEditorZoomContext.screenWidth() > 0) && (_imp->curveEditorZoomContext.screenHeight() > 0) ) {

            _imp->curveEditorZoomContext.fill( xmin, xmax, _imp->curveEditorZoomContext.bottom(), _imp->curveEditorZoomContext.top() );
        }
    }
    update();
}


void
AnimationModuleView::refreshSelectionBoundingBox()
{
    _imp->refreshCurveEditorSelectedKeysBRect();
    _imp->refreshDopeSheetSelectedKeysBRect();
}

void
AnimationModuleView::refreshSelectionBboxAndRedraw()
{
    refreshSelectionBoundingBox();
    update();
}

void
AnimationModuleView::onSelectionModelKeyframeSelectionChanged()
{
    refreshSelectionBboxAndRedraw();
}

void
AnimationModuleView::onUpdateOnPenUpActionTriggered()
{
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    appPTR->getCurrentSettings()->setRenderOnEditingFinishedOnly(!updateOnPenUpOnly);
}

void
AnimationModuleView::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool accept = true;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    AnimationModuleBasePtr model = _imp->_model.lock();
    if (!model) {
        return;
    }
    if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleRemoveKeys, modifiers, key) ) {
        onRemoveSelectedKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleConstant, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeConstant);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleLinear, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeLinear);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSmooth, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCatmullrom, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCubic, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeCubic);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleHorizontal, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleBreak, modifiers, key) ) {
        model->setSelectedKeysInterpolation(eKeyframeTypeBroken);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenterAll, modifiers, key) ) {
        onCenterAllCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenter, modifiers, key) ) {
        onCenterOnSelectedCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSelectAll, modifiers, key) ) {
        onSelectAllKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCopy, modifiers, key) ) {
        onCopySelectedKeyFramesToClipBoardActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframes, modifiers, key) ) {
        onPasteClipBoardKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframesAbsolute, modifiers, key) ) {
        onPasteClipBoardKeyFramesAbsoluteActionTriggered();
    } else if ( key == Qt::Key_Plus ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( key == Qt::Key_Minus ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else {
        accept = false;
    }

    AnimationModulePtr isAnimModule = toAnimationModule(model);
    if (accept) {
        if (isAnimModule) {
            isAnimModule->getEditor()->onInputEventCalled();
        }

        e->accept();
    } else {
        if (isAnimModule) {
            isAnimModule->getEditor()->handleUnCaughtKeyPressEvent(e);
        }
        QGLWidget::keyPressEvent(e);
    }
} // keyPressEvent

void
AnimationModuleView::mousePressEvent(QMouseEvent* e)
{

    // Need to set focus otherwise some of the keypress handlers will not be called
    setFocus();

    AnimationModuleBasePtr model = _imp->_model.lock();
    AnimationModulePtr isAnimModule = toAnimationModule(model);
    if (isAnimModule) {
        isAnimModule->getEditor()->onInputEventCalled();
    }


    _imp->lastMousePos = e->pos();
    _imp->dragStartPoint = e->pos();
    _imp->keyDragLastMovement.ry() = _imp->keyDragLastMovement.rx() = 0;

    _imp->mustSetDragOrientation = true;


    AnimationModuleEditor::AnimationModuleDisplayViewMode displayMode = AnimationModuleEditor::eAnimationModuleDisplayViewModeCurveEditor;
    if (isAnimModule) {
        displayMode = isAnimModule->getEditor()->getDisplayMode();
    }

    int treeWidgetBottomY = _imp->treeView && displayMode == AnimationModuleEditor::eAnimationModuleDisplayViewModeStacked ? _imp->treeView->getTreeBottomYWidgetCoords() : 0;
    bool isInsideCurveEditor = e->y() >= treeWidgetBottomY;

    _imp->eventTriggeredFromCurveEditor = isInsideCurveEditor;

    // right button: popup menu
    if ( buttonDownIsRight(e) ) {
        int treeBottomItemY = _imp->treeView ? _imp->treeView->getTreeBottomYWidgetCoords() : 0;
        bool isInsideCurveEditor = e->y() > treeBottomItemY;
        _imp->createMenu(isInsideCurveEditor, mapToGlobal(e->pos()));
        e->accept();
        return;
    }

    // middle button: scroll view
    if ( buttonDownIsMiddle(e) ) {
        _imp->state = eEventStateDraggingView;
        e->accept();
        return;
    } else if ( ( (e->buttons() & Qt::MiddleButton) &&
                 ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
               ( (e->buttons() & Qt::LeftButton) &&
                ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) {
                   // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
                   _imp->state = eEventStateZooming;
                   e->accept();
                   return;
               }

    if (isInsideCurveEditor && _imp->curveEditorMousePressEvent(e)) {
        e->accept();
        update();
        return;
    } else {
        if (_imp->dopeSheetMousePressEvent(e)) {
            e->accept();
            update();
            return;
        }
    }


    // is the click near the vertical current time marker?
    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
        _imp->state = eEventStateDraggingTimeline;
        e->accept();
        update();
        return;
    }


    // default behaviour: unselect selected keyframes, if any, and start a new selection
    if ( !modCASIsControl(e) ) {
        model->getSelectionModel()->clearSelection();
    }


    _imp->state = isInsideCurveEditor ? eEventStateSelectionRectCurveEditor : eEventStateSelectionRectDopeSheet;
    e->accept();
    
} // mousePressEvent

void
AnimationModuleView::mouseMoveEvent(QMouseEvent* e)
{
    QPoint evPos = e->pos();
    if (!_imp->setCurveEditorCursor(evPos)) {
        if (!_imp->setDopeSheetCursor(evPos)) {
            unsetCursor();
        }
    }


    AnimationModuleBasePtr model = _imp->_model.lock();
    AnimationModulePtr isAnimModule = toAnimationModule(model);
    if (_imp->state == eEventStateNone) {

        // nothing else to do
        TabWidget* tab = 0;
        if (isAnimModule) {
            tab = isAnimModule->getEditor()->getParentPane();
        }
        if (tab) {
            // If the Viewer is in a tab, send the tab widget the event directly
            qApp->sendEvent(tab, e);
        } else {
            QGLWidget::mouseMoveEvent(e);
        }

        return;
    }


    if (_imp->mustSetDragOrientation) {
        QPointF diff(e->pos() - _imp->dragStartPoint);
        double dist = diff.manhattanLength();
        if (dist > 5) {
            if (isAnimModule || modCASIsControl(e)) {
                if ( std::abs( diff.x() ) > std::abs( diff.y() ) ) {
                    _imp->mouseDragOrientation.setX(1);
                    _imp->mouseDragOrientation.setY(0);
                } else {
                    _imp->mouseDragOrientation.setX(0);
                    _imp->mouseDragOrientation.setY(1);
                }
            } else {
                _imp->mouseDragOrientation.rx() = _imp->mouseDragOrientation.ry() = 1;
            }
            _imp->mustSetDragOrientation = false;

            }

    }

    switch (_imp->state) {
        case eEventStateDraggingView: {
            QPointF newClick_opengl, oldClick_opengl;
            newClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( e->x(), e->y() );
            oldClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->lastMousePos.y() );

            _imp->zoomOrPannedSinceLastFit = true;
            double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
            double dy = ( oldClick_opengl.y() - newClick_opengl.y() );


            {
                QMutexLocker k(&_imp->zoomCtxMutex);
                _imp->curveEditorZoomContext.translate(dx, dy);
                _imp->dopeSheetZoomContext.translate(dx, 0);
            }

            // Synchronize the opened viewers
            if ( _imp->_gui->isTripleSyncEnabled() ) {
                _imp->_gui->centerOpenedViewersOn( _imp->curveEditorZoomContext.left(), _imp->curveEditorZoomContext.right() );
            }
            e->accept();
            update();
            _imp->lastMousePos = e->pos();
            break;
        }
        case eEventStateDraggingTimeline: {
            _imp->moveCurrentFrameIndicator((int)_imp->curveEditorZoomContext.toZoomCoordinates( e->x(), 0 ).x());
            e->accept();
            update();
            break;
        }
        case eEventStateDraggingKeys: {
            if (!_imp->mustSetDragOrientation) {
                QPointF newClick_opengl, oldClick_opengl;
                if (_imp->eventTriggeredFromCurveEditor) {
                    newClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( e->x(), e->y() );
                    oldClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->lastMousePos.y() );
                } else {
                    // Use a delta y of 0
                    newClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( e->x(), _imp->dragStartPoint.y() );
                    oldClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->dragStartPoint.y() );
                }
                _imp->moveSelectedKeyFrames(oldClick_opengl, newClick_opengl);
            }
        }   break;
        case eEventStateDraggingBtmLeftBbox:
        case eEventStateDraggingMidBtmBbox:
        case eEventStateDraggingBtmRightBbox:
        case eEventStateDraggingMidRightBbox:
        case eEventStateDraggingTopRightBbox:
        case eEventStateDraggingMidTopBbox:
        case eEventStateDraggingTopLeftBbox:
        case eEventStateDraggingMidLeftBbox: {
            QPointF newClick_opengl, oldClick_opengl;
            if (_imp->eventTriggeredFromCurveEditor) {
                newClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( e->x(), e->y() );
                oldClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->lastMousePos.y() );
            } else {
                newClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( e->x(), e->y() );
                oldClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->lastMousePos.y() );
            }


            AnimationModuleViewPrivate::TransformSelectionCenterEnum centerType;
            if (_imp->state == eEventStateDraggingMidBtmBbox) {
                centerType = AnimationModuleViewPrivate::eTransformSelectionCenterTop;
            } else if (_imp->state == eEventStateDraggingMidLeftBbox) {
                centerType = AnimationModuleViewPrivate::eTransformSelectionCenterRight;
            } else if (_imp->state == eEventStateDraggingMidTopBbox) {
                centerType = AnimationModuleViewPrivate::eTransformSelectionCenterBottom;
            } else if (_imp->state == eEventStateDraggingMidRightBbox) {
                centerType = AnimationModuleViewPrivate::eTransformSelectionCenterLeft;
            } else {
                centerType = AnimationModuleViewPrivate::eTransformSelectionCenterMiddle;
            }
            bool scaleBoth = _imp->state == eEventStateDraggingTopLeftBbox ||
            _imp->state == eEventStateDraggingTopRightBbox ||
            _imp->state == eEventStateDraggingBtmRightBbox ||
            _imp->state == eEventStateDraggingBtmLeftBbox;

            bool scaleX = scaleBoth ||
            _imp->state == eEventStateDraggingMidLeftBbox ||
            _imp->state == eEventStateDraggingMidRightBbox;


            bool scaleY = scaleBoth ||
            _imp->state == eEventStateDraggingMidTopBbox ||
            _imp->state == eEventStateDraggingMidBtmBbox;

            _imp->transformSelectedKeyFrames( oldClick_opengl, newClick_opengl, modCASIsShift(e), centerType, scaleX, scaleY );
            e->accept();
            update();
            break;
        }
        case eEventStateZooming: {
            int deltaX = 2 * ( e->x() - _imp->lastMousePos.x() );
            int deltaY = -2 * ( e->y() - _imp->lastMousePos.y() );
            _imp->zoomOrPannedSinceLastFit = true;

            _imp->zoomView(_imp->dragStartPoint, deltaX, deltaY, _imp->curveEditorZoomContext, true, true);
            _imp->zoomView(_imp->dragStartPoint, deltaX, 0, _imp->dopeSheetZoomContext, true, false);

            // Wheel: zoom values and time, keep point under mouse
            // Synchronize the dope sheet editor and opened viewers
            if ( _imp->_gui->isTripleSyncEnabled() ) {
                _imp->_gui->centerOpenedViewersOn( _imp->curveEditorZoomContext.left(), _imp->curveEditorZoomContext.right() );
            }
            e->accept();
            update();
            break;
        }
        case eEventStateSelectionRectCurveEditor: {
            QPointF newClick_opengl, oldClick_opengl;
            newClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( e->x(), e->y() );
            oldClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->lastMousePos.y() );
            _imp->refreshSelectionRectangle(_imp->curveEditorZoomContext, newClick_opengl.x(), newClick_opengl.y() );
            e->accept();
            update();
            break;
        }
        case eEventStateSelectionRectDopeSheet: {
            QPointF newClick_opengl, oldClick_opengl;
            newClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( e->x(), e->y() );
            oldClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( _imp->lastMousePos.x(), _imp->lastMousePos.y() );
            _imp->refreshSelectionRectangle(_imp->dopeSheetZoomContext,  newClick_opengl.x(), newClick_opengl.y() );
            e->accept();
            update();
            break;
        }
        case eEventStateDraggingTangent: {
            if (_imp->_gui) {
                _imp->_gui->setDraftRenderEnabled(true);
            }
            QPointF newClick_opengl;
            newClick_opengl = _imp->curveEditorZoomContext.toZoomCoordinates( e->x(), e->y() );
            _imp->moveSelectedTangent(newClick_opengl);
            e->accept();
            update();
            break;;
        }
        case eEventStateReaderLeftTrim: {
            QPointF newClick_opengl;
            newClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( e->x(), e->y() );
            KnobIntPtr timeOffsetKnob = toKnobInt(_imp->currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset));
            assert(timeOffsetKnob);
            double newFirstFrame = std::floor(newClick_opengl.x() - timeOffsetKnob->getValue() + 0.5);
            model->trimReaderLeft(_imp->currentEditedReader, newFirstFrame);
            e->accept();
            update();
            break;
        }
        case eEventStateReaderRightTrim: {
            QPointF newClick_opengl;
            newClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( e->x(), e->y() );
            KnobIntPtr timeOffsetKnob = toKnobInt(_imp->currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset));
            assert(timeOffsetKnob);
            double newLastFrame = std::floor(newClick_opengl.x() - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderRight(_imp->currentEditedReader, newLastFrame);
            e->accept();
            update();
            break;
        }
        case eEventStateReaderSlip: {
            QPointF newClick_opengl = _imp->dopeSheetZoomContext.toZoomCoordinates( e->x(), e->y() );
            QPointF dragStartPointOpenGL = _imp->dopeSheetZoomContext.toZoomCoordinates( _imp->dragStartPoint.x(), _imp->dragStartPoint.y() );

            double deltaSinceDragStart = std::floor(newClick_opengl.x() - dragStartPointOpenGL.x() + 0.5);
            double dt = deltaSinceDragStart - _imp->keyDragLastMovement.x();
            // And update _keyDragLastMovement
            _imp->keyDragLastMovement.rx() = deltaSinceDragStart;
            model->slipReader(_imp->currentEditedReader, dt);
            e->accept();
            update();
            break;
        }
        case eEventStateNone:
            break;
    } // switch

    _imp->lastMousePos = e->pos();


} // mouseMoveEvent

void
AnimationModuleView::mouseReleaseEvent(QMouseEvent* e)
{

    // If we were dragging timeline and autoproxy is on, then on mouse release trigger a non proxy render
    if ( _imp->_gui->isDraftRenderEnabled() ) {
        _imp->_gui->setDraftRenderEnabled(false);
        bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
        if (autoProxyEnabled) {
            _imp->_gui->renderAllViewers();
        }
    }


    bool mustUpdate = false;
    if (_imp->state == eEventStateSelectionRectCurveEditor) {
        _imp->makeSelectionFromCurveEditorSelectionRectangle(modCASIsShift(e));
        // We must redraw to remove the selection rectangle
        mustUpdate = true;
    } else if (_imp->state == eEventStateSelectionRectDopeSheet) {
        _imp->makeSelectionFromDopeSheetSelectionRectangle(modCASIsShift(e));
        // We must redraw to remove the selection rectangle
        mustUpdate = true;
    }

    _imp->currentEditedReader.reset();
    _imp->selectionRect.clear();

    _imp->state = eEventStateNone;

    if (mustUpdate) {
        update();
    }

} // mouseReleaseEvent

void
AnimationModuleView::mouseDoubleClickEvent(QMouseEvent* e)
{

    if (_imp->curveEditorDoubleClickEvent(e)) {
        e->accept();
        update();
        return;
    }
    if (_imp->dopeSheetDoubleClickEvent(e)) {
        e->accept();
        update();
        return;
    }
} // mouseDoubleClickEvent

void
AnimationModuleView::wheelEvent(QWheelEvent *e)
{
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    setFocus();

    if ( modCASIsControlShift(e) ) {
        // Values only
        _imp->zoomView(e->pos(), e->delta(), e->delta(), _imp->curveEditorZoomContext, false, true);
    } else if ( modCASIsControl(e) ) {
        // Times only
        _imp->zoomView(e->pos(), e->delta(), e->delta(), _imp->curveEditorZoomContext, true, false);
        _imp->zoomView(e->pos(), e->delta(), e->delta(), _imp->dopeSheetZoomContext, true, false);
    } else {
        // Values + Times
        _imp->zoomView(e->pos(), e->delta(), e->delta(), _imp->curveEditorZoomContext, true, true);
        _imp->zoomView(e->pos(), e->delta(), e->delta(), _imp->dopeSheetZoomContext, true, false);
    }


    // Synchronize the dope sheet editor and opened viewers
    if ( _imp->_gui->isTripleSyncEnabled() ) {
        _imp->_gui->centerOpenedViewersOn( _imp->curveEditorZoomContext.left(), _imp->curveEditorZoomContext.right() );
    }
    _imp->zoomOrPannedSinceLastFit = true;

    update();
} // wheelEvent

QPointF
AnimationModuleView::toZoomCoordinates(double x, double y) const
{
    return _imp->curveEditorZoomContext.toZoomCoordinates(x, y);
}
QPointF
AnimationModuleView::toWidgetCoordinates(double x, double y) const
{
    return _imp->curveEditorZoomContext.toWidgetCoordinates(x, y);
}

void
AnimationModuleView::setSelectedDerivative(MoveTangentCommand::SelectedTangentEnum tangent, AnimItemDimViewKeyFrame key)
{
    _imp->selectedDerivative.first = tangent;
    _imp->selectedDerivative.second = key;
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_AnimationModuleView.cpp"

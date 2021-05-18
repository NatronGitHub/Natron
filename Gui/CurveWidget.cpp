/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "CurveWidget.h"

#include <cmath> // floor
#include <stdexcept>
#include <sstream> // stringstream

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QApplication>
#include <QToolButton>
#include <QDesktopWidget>

#include "Engine/Bezier.h"
#include "Engine/PyParameter.h" // IntParam
#include "Engine/Project.h"
#include "Engine/Image.h"
#include "Engine/RotoContext.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidgetDialogs.h"
#include "Gui/CurveWidgetPrivate.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"

#include "Gui/KnobGui.h"
#include "Gui/PythonPanels.h" // PyModelDialog
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER

/*****************************CURVE WIDGET***********************************************/


bool
CurveWidget::isSelectedKey(const CurveGuiPtr& curve,
                           double time) const
{
    SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.find(curve);

    if ( it == _imp->_selectedKeyFrames.end() ) {
        return false;
    }

    for (std::list<KeyPtr>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
        if ( ( time >= ( (*it2)->key.getTime() - 1e-6 ) ) && ( time <= ( (*it2)->key.getTime() + 1e-6 ) ) ) {
            return true;
        }
    }


    return false;
}

void
CurveWidget::pushUndoCommand(QUndoCommand* cmd)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(cmd);
}

QUndoStack*
CurveWidget::getUndoStack() const
{
    return _imp->_undoStack.get();
}

///////////////////////////////////////////////////////////////////
// CurveWidget
//

CurveWidget::CurveWidget(Gui* gui,
                         CurveSelection* selection,
                         TimeLinePtr timeline,
                         QWidget* parent,
                         const QGLWidget* shareWidget)
    : QGLWidget(parent, shareWidget)
    , _imp( new CurveWidgetPrivate(gui, selection, timeline, this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    if (timeline) {
        ProjectPtr project = gui->getApp()->getProject();
        assert(project);
        QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimeLineFrameChanged(SequenceTime,int)) );
        QObject::connect( project.get(), SIGNAL(frameRangeChanged(int,int)), this, SLOT(onTimeLineBoundariesChanged(int,int)) );
        onTimeLineFrameChanged(timeline->currentFrame(), eValueChangedReasonNatronGuiEdited);

        double left, right;
        project->getFrameRange(&left, &right);
        onTimeLineBoundariesChanged(left, right);
    }

    if ( parent->objectName() == QString::fromUtf8("CurveEditorSplitter") ) {
        ///if this is the curve widget associated to the CurveEditor
        //        QDesktopWidget* desktop = QApplication::desktop();
        //        _imp->sizeH = desktop->screenGeometry().size();
        _imp->sizeH = QSize(10000, 10000);
    } else {
        ///a random parametric param curve editor
        _imp->sizeH =  QSize(400, 400);
    }
}

CurveWidget::~CurveWidget()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();
}

void
CurveWidget::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    appPTR->initializeOpenGLFunctionsOnce();
}

void
CurveWidget::addCurveAndSetColor(const CurveGuiPtr& curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //update(); //force initializeGL to be called if it wasn't before.
    _imp->_curves.push_back(curve);
    curve->setColor(_imp->_nextCurveAddedColor);
    _imp->_nextCurveAddedColor.setHsv( _imp->_nextCurveAddedColor.hsvHue() + 60,
                                       _imp->_nextCurveAddedColor.hsvSaturation(), _imp->_nextCurveAddedColor.value() );
}

void
CurveWidget::removeCurve(CurveGui *curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if (it->get() == curve) {
            //remove all its keyframes from selected keys
            SelectedKeys::iterator found = _imp->_selectedKeyFrames.find(*it);
            if ( found != _imp->_selectedKeyFrames.end() ) {
                _imp->_selectedKeyFrames.erase(found);
            }

            _imp->_curves.erase(it);
            break;
        }
    }
}

void
CurveWidget::centerOn(const std::vector<CurveGuiPtr> & curves, bool useDisplayRange)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // First try to center curves given their display range
    Curve::YRange displayRange(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    std::pair<double, double> xRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    bool rangeSet = false;

    std::vector<CurveGuiPtr> curvesToFrame;
    if (curves.empty()) {
        curvesToFrame.reserve( _imp->_curves.size() );
        curvesToFrame.insert( curvesToFrame.end(), _imp->_curves.begin(), _imp->_curves.end() );
        assert(curvesToFrame.size() == _imp->_curves.size());
    } else {
        curvesToFrame = curves;
    }

    if (curvesToFrame.empty()) {
        return;
    }

    if (useDisplayRange) {
        for (std::vector<CurveGuiPtr> ::const_iterator it = curvesToFrame.begin(); it != curvesToFrame.end(); ++it) {
            CurvePtr curve = (*it)->getInternalCurve();
            if (!curve) {
                continue;
            }
            Curve::YRange thisCurveRange = curve->getCurveDisplayYRange();
            std::pair<double, double> thisXRange = curve->getXRange();


            if (thisCurveRange.min == -std::numeric_limits<double>::infinity() ||
                thisCurveRange.min == INT_MIN ||
                thisCurveRange.max == std::numeric_limits<double>::infinity() ||
                thisCurveRange.max == INT_MAX ||
                thisXRange.first == -std::numeric_limits<double>::infinity() ||
                thisXRange.first == INT_MIN ||
                thisXRange.second == std::numeric_limits<double>::infinity() ||
                thisXRange.second == INT_MAX) {
                continue;
            }


            if (!rangeSet) {
                displayRange = thisCurveRange;
                xRange = thisXRange;
                rangeSet = true;
            } else {
                displayRange.min = std::min(displayRange.min, thisCurveRange.min);
                displayRange.max = std::min(displayRange.max, thisCurveRange.max);
                xRange.first = std::min(xRange.first, thisXRange.first);
                xRange.second = std::min(xRange.second, thisXRange.second);
            }
        } // for all curves

        if (rangeSet) {
            double paddingX = (xRange.second - xRange.first) / 20.;
            double paddingY = (displayRange.max - displayRange.min) / 20.;
            centerOn(xRange.first - paddingX, xRange.second + paddingX, displayRange.min - paddingY, displayRange.max + paddingY);
            return;
        }
    } // useDisplayRange
    
    // If no range, center them using the bounding box of keyframes
    
    bool doCenter = false;
    RectD ret;
    for (U32 i = 0; i < curvesToFrame.size(); ++i) {
        const CurveGuiPtr& c = curvesToFrame[i];
        KeyFrameSet keys = c->getKeyFrames();

        if ( keys.empty() ) {
            continue;
        }
        doCenter = true;
        double xmin = keys.begin()->getTime();
        double xmax = keys.rbegin()->getTime();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            double value = it2->getValue();
            if (value < ymin) {
                ymin = value;
            }
            if (value > ymax) {
                ymax = value;
            }
        }
        ret.merge(xmin, ymin, xmax, ymax);
    }
    ret.set_bottom(ret.bottom() - ret.height() / 10);
    ret.set_left(ret.left() - ret.width() / 10);
    ret.set_right(ret.right() + ret.width() / 10);
    ret.set_top(ret.top() + ret.height() / 10);
    if ( doCenter && !ret.isNull() ) {
        centerOn( ret.left(), ret.right(), ret.bottom(), ret.top() );
    }
}

void
CurveWidget::showCurvesAndHideOthers(const std::vector<CurveGuiPtr> & curves)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<CurveGuiPtr>::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        std::vector<CurveGuiPtr>::const_iterator it2 = std::find(curves.begin(), curves.end(), *it);

        if ( it2 != curves.end() ) {
            (*it)->setVisible(true);
        } else {
            (*it)->setVisible(false);
        }
    }
    update();
}

void
CurveWidget::updateSelectionAfterCurveChange(CurveGui* curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///check whether selected keyframes have changed
    ///we cannot use std::transform here because a keyframe might have disappeared from a curve
    ///hence the number of keyframes selected would decrease

    SelectedKeys::iterator found = _imp->_selectedKeyFrames.end();
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        if (it->first.get() == curve) {
            found = it;
            break;
        }
    }
    if ( found == _imp->_selectedKeyFrames.end() ) {
        return;
    }

    CurvePtr internalCurve = found->first->getInternalCurve();
    bool isPeriodic = false;
    std::pair<double,double> parametricRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    if (internalCurve) {
        isPeriodic = internalCurve->isCurvePeriodic();
        parametricRange = internalCurve->getXRange();
    }


    KeyFrameSet set = found->first->getKeyFrames();
    std::list<KeyPtr> newSelection;
    for (std::list<KeyPtr>::iterator it2 = found->second.begin(); it2 != found->second.end(); ++it2) {
        KeyFrameSet::const_iterator found = Curve::findWithTime( set, (*it2)->key.getTime() );
        if ( found != set.end() ) {
            (*it2)->key = *found;
            KeyFrameSet::const_iterator next = found;
            ++next;
            if ( next != set.end() ) {
                (*it2)->nextKey = *next;
                (*it2)->hasNext = true;
            } else if (isPeriodic) {
                KeyFrameSet::const_iterator start = set.begin();
                (*it2)->nextKey = *start;
                (*it2)->nextKey.setTime((*it2)->nextKey.getTime() + (parametricRange.second - parametricRange.first));
                (*it2)->hasNext = true;
            }
            if ( found != set.begin()) {
                KeyFrameSet::const_iterator prev = found;
                --prev;
                (*it2)->prevKey = *prev;
                (*it2)->hasPrevious = true;
            } else if (isPeriodic) {
                KeyFrameSet::const_reverse_iterator last = set.rbegin();
                (*it2)->prevKey = *last;
                (*it2)->prevKey.setTime((*it2)->prevKey.getTime() - (parametricRange.second - parametricRange.first));
                (*it2)->hasPrevious = true;
            }

            newSelection.push_back(*it2);
        }
    }

    found->second = newSelection;

    refreshCurveDisplayTangents(curve);
    refreshSelectedKeysBbox();
}

void
CurveWidget::getVisibleCurves(std::vector<CurveGuiPtr>* curves) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<CurveGuiPtr>::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            curves->push_back(*it);
        }
    }
}

void
CurveWidget::centerOn(double xmin,
                      double xmax,
                      double ymin,
                      double ymax)

{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( (_imp->zoomCtx.screenWidth() > 0) && (_imp->zoomCtx.screenHeight() > 0) ) {
        _imp->zoomCtx.fill(xmin, xmax, ymin, ymax);
    }
    _imp->zoomOrPannedSinceLastFit = false;


    refreshDisplayedTangents();
    update();
}

/**
 * @brief Swap the OpenGL buffers.
 **/
void
CurveWidget::swapOpenGLBuffers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    swapBuffers();
}

/**
 * @brief Repaint
 **/
void
CurveWidget::redraw()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    update();
}

/**
 * @brief Returns the width and height of the viewport in window coordinates.
 **/
void
CurveWidget::getViewportSize(double &width,
                             double &height) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    width = this->width();
    height = this->height();
}

/**
 * @brief Returns the pixel scale of the viewport.
 **/
void
CurveWidget::getPixelScale(double & xScale,
                           double & yScale) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    xScale = _imp->zoomCtx.screenPixelWidth();
    yScale = _imp->zoomCtx.screenPixelHeight();
}

#ifdef OFX_EXTENSIONS_NATRON
double
CurveWidget::getScreenPixelRatio() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

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
CurveWidget::getBackgroundColour(double &r,
                                 double &g,
                                 double &b) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);
}

RectD
CurveWidget::getViewportRect() const
{
    RectD bbox;
    {
        bbox.x1 = _imp->zoomCtx.left();
        bbox.y1 = _imp->zoomCtx.bottom();
        bbox.x2 = _imp->zoomCtx.right();
        bbox.y2 = _imp->zoomCtx.top();
    }

    return bbox;
}

void
CurveWidget::getCursorPosition(double& x,
                               double& y) const
{
    QPoint p = QCursor::pos();

    p = mapFromGlobal(p);
    QPointF mappedPos = toZoomCoordinates( p.x(), p.y() );
    x = mappedPos.x();
    y = mappedPos.y();
}

void
CurveWidget::saveOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack();
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
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

void
CurveWidget::restoreOpenGLContext()
{
    assert( QThread::currentThread() == qApp->thread() );

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
CurveWidget::resizeGL(int width,
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
    glViewport (0, 0, width, height);

    // Width and height may be 0 when tearing off a viewer tab to another panel
    if ( (width > 0) && (height > 0) ) {
        _imp->zoomCtx.setScreenSize(width, height);
    }

    if (height == 1) {
        //don't do the following when the height of the widget is irrelevant
        return;
    }

    if (!_imp->zoomOrPannedSinceLastFit) {
        centerOn(std::vector<CurveGuiPtr>(), true);
    }
}

void
CurveWidget::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError();
    if (_imp->zoomCtx.factor() <= 0) {
        return;
    }
    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomCtx.left();
    zoomRight = _imp->zoomCtx.right();
    zoomBottom = _imp->zoomCtx.bottom();
    zoomTop = _imp->zoomCtx.top();

    double bgR, bgG, bgB;
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&bgR, &bgG, &bgB);

    if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

        return;
    }

    {
        //GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        //GLProtectMatrix p(GL_PROJECTION);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);
        //GLProtectMatrix m(GL_MODELVIEW);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glCheckError();

        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

        OfxParamOverlayInteractPtr customInteract = getCustomInteract();
        if (customInteract) {
            // Don't protect GL_COLOR_BUFFER_BIT, because it seems to hit an OpenGL bug on
            // some macOS configurations (10.10-10.12), where garbage is displayed in the viewport.
            // see https://github.com/MrKepzie/Natron/issues/1460
            //GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
            GLProtectAttrib a(GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

            RenderScale scale(1.);
            customInteract->setCallingViewport(this);
            customInteract->drawAction(0, scale, 0, customInteract->hasColorPicker() ? &customInteract->getLastColorPickerColor() : 0);
            glCheckErrorIgnoreOSXBug();
        }

        _imp->drawScale();



        if (_imp->_timelineEnabled) {
            _imp->drawTimelineMarkers();
        }

        if (_imp->_drawSelectedKeyFramesBbox) {
            _imp->drawSelectedKeyFramesBbox();
        }

        _imp->drawCurves();

        if ( !_imp->_selectionRectangle.isNull() ) {
            _imp->drawSelectionRectangle();
        }
    } // GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
    glCheckError();
} // CurveWidget::paintGL

bool
CurveWidget::renderText(double x,
                        double y,
                        const std::string &string,
                        double r,
                        double g,
                        double b,
                        int flags)
{
    QColor c;

    c.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    renderText( x, y, QString::fromUtf8( string.c_str() ), c, font(), flags );

    return true;
}

void
CurveWidget::renderText(double x,
                        double y,
                        const QString & text,
                        const QColor & color,
                        const QFont & font,
                        int flags) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( text.isEmpty() ) {
        return;
    }

    double w = (double)width();
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom();
    double left = _imp->zoomCtx.left();
    double top =  _imp->zoomCtx.top();
    double right = _imp->zoomCtx.right();
    if ( (w <= 0) || (h <= 0) || (right <= left) || (top <= bottom) ) {
        return;
    }
    double scalex = (right - left) / w;
    double scaley = (top - bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font, flags);
    glCheckError();
}

void
CurveWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///If the click is on a curve but not nearby a keyframe, add a keyframe


    CurveGuiPtr selectedKeyCurve;
    KeyFrame selectedKey, selectedKeyPrev, selectedKeyNext;
    bool selectedKeyHasPrev, selectedKeyHasNext;
    bool hasSelectedKey = _imp->isNearbyKeyFrame(e->pos(), &selectedKeyCurve, &selectedKey, &selectedKeyHasPrev, &selectedKeyPrev,
                                                 &selectedKeyHasNext, &selectedKeyNext);
    std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> selectedTan = _imp->isNearbyTangent( e->pos() );
    if (hasSelectedKey || selectedTan.second) {
        return;
    }


    KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( selectedKeyCurve.get() );
    if (isKnobCurve) {
        KnobGuiPtr knobUI = isKnobCurve->getKnobGui();
        if (knobUI) {
            int curveDim = isKnobCurve->getDimension();
            KnobIPtr internalKnob = knobUI->getKnob();
            if ( internalKnob && ( !internalKnob->isEnabled(curveDim) || internalKnob->isSlave(curveDim) ) ) {
                return;
            }
        }
    }

    EditKeyFrameDialog::EditModeEnum mode = EditKeyFrameDialog::eEditModeKeyframePosition;
    KeyPtr selectedText;
    ///We're nearby a selected keyframe's text
    KeyPtr keyText = _imp->isNearbyKeyFrameText( e->pos() );
    if (keyText) {
        selectedText = keyText;
    } else {
        std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> tangentText = _imp->isNearbySelectedTangentText( e->pos() );
        if (tangentText.second) {
            if (tangentText.first == MoveTangentCommand::eSelectedTangentLeft) {
                mode = EditKeyFrameDialog::eEditModeLeftDerivative;
            } else {
                mode = EditKeyFrameDialog::eEditModeRightDerivative;
            }
            selectedText = tangentText.second;
        }
    }


    if (selectedText) {
        EditKeyFrameDialog* dialog = new EditKeyFrameDialog(mode, this, selectedText, this);
        int dialogW = dialog->sizeHint().width();
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        QPoint gP = e->globalPos();
        if ( gP.x() > (screen.width() - dialogW) ) {
            gP.rx() -= dialogW;
        }

        dialog->move(gP);

        ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
        ///it closes it.
        QObject::connect( dialog, SIGNAL(accepted()), this, SLOT(onEditKeyFrameDialogFinished()) );
        QObject::connect( dialog, SIGNAL(rejected()), this, SLOT(onEditKeyFrameDialogFinished()) );
        dialog->show();

        e->accept();

        return;
    }

    ////
    // is the click near a curve?
    double xCurve, yCurve;
    Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
    if ( foundCurveNearby != _imp->_curves.end() ) {
        addKey(*foundCurveNearby, xCurve, yCurve);

        _imp->_keyDragLastMovement.rx() = 0.;
        _imp->_keyDragLastMovement.ry() = 0.;
        _imp->_dragStartPoint = e->pos();
        _imp->_lastMousePos = e->pos();
        e->accept();

        return;
    }
} // CurveWidget::mouseDoubleClickEvent

void
CurveWidget::onEditKeyFrameDialogFinished()
{
    EditKeyFrameDialog* dialog = qobject_cast<EditKeyFrameDialog*>( sender() );

    if (dialog) {
        //QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        dialog->deleteLater();
    }
}

//
// Decide what should be done in response to a mouse press.
// When the reason is found, process it and return.
// (this function has as many return points as there are reasons)
//
void
CurveWidget::mousePressEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    CurveEditor* ce = 0;
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if ( parent->objectName() == QString::fromUtf8("CurveEditor") ) {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (ce) {
        ce->onInputEventCalled();
    }

    setFocus();
    ////
    // right button: popup menu
    if ( buttonDownIsRight(e) ) {
        _imp->createMenu();
        _imp->_rightClickMenu->exec( mapToGlobal( e->pos() ) );
        _imp->_dragStartPoint = e->pos();
        // no need to set _imp->_lastMousePos
        // no need to set _imp->_dragStartPoint

        // no need to update()
        e->accept();

        return;
    }

    if ( modCASIsControlAlt(e) ) { // Ctrl+Alt (Cmd+Alt on Mac) = insert keyframe
        ////
        // is the click near a curve?
        double xCurve, yCurve;
        Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
        if ( foundCurveNearby != _imp->_curves.end() ) {
            addKey(*foundCurveNearby, xCurve, yCurve);
            _imp->_keyDragLastMovement.rx() = 0.;
            _imp->_keyDragLastMovement.ry() = 0.;
            _imp->_dragStartPoint = e->pos();
            _imp->_lastMousePos = e->pos();
        }
        e->accept();

        return;
    }

    ////
    // middle button: scroll view
    if ( buttonDownIsMiddle(e) ) {
        _imp->_state = eEventStateDraggingView;
        _imp->_lastMousePos = e->pos();
        _imp->_dragStartPoint = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to update()
        e->accept();

        return;
    } else if ( ( (e->buttons() & Qt::MiddleButton) &&
                  ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
                ( (e->buttons() & Qt::LeftButton) &&
                  ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->_state = eEventStateZooming;
        _imp->_lastMousePos = e->pos();
        _imp->_dragStartPoint = e->pos();

        e->accept();

        return;
    }

    // is the click near the multiple-keyframes selection box center?
    if (_imp->_drawSelectedKeyFramesBbox) {
        bool caughtBbox = true;
        if ( _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
            _imp->_state = eEventStateDraggingKeys;
        } else if ( _imp->isNearbyBboxBtmLeft( e->pos() ) ) {
            _imp->_state = eEventStateDraggingBtmLeftBbox;
        } else if ( _imp->isNearbyBboxMidLeft( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidLeftBbox;
        } else if ( _imp->isNearbyBboxTopLeft( e->pos() ) ) {
            _imp->_state = eEventStateDraggingTopLeftBbox;
        } else if ( _imp->isNearbyBboxMidTop( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidTopBbox;
        } else if ( _imp->isNearbyBboxTopRight( e->pos() ) ) {
            _imp->_state = eEventStateDraggingTopRightBbox;
        } else if ( _imp->isNearbyBboxMidRight( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidRightBbox;
        } else if ( _imp->isNearbyBboxBtmRight( e->pos() ) ) {
            _imp->_state = eEventStateDraggingBtmRightBbox;
        } else if ( _imp->isNearbyBboxMidBtm( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidBtmBbox;
        } else {
            caughtBbox = false;
        }
        if (caughtBbox) {
            _imp->_mustSetDragOrientation = true;
            _imp->_keyDragLastMovement.rx() = 0.;
            _imp->_keyDragLastMovement.ry() = 0.;
            _imp->_dragStartPoint = e->pos();
            _imp->_lastMousePos = e->pos();

            //no need to update()
            e->accept();

            return;
        }
    }
    ////
    // is the click near a keyframe manipulator?
    CurveGuiPtr selectedKeyCurve;
    KeyFrame selectedKey, selectedKeyPrev, selectedKeyNext;
    bool selectedKeyHasPrev, selectedKeyHasNext;
    bool hasSelectedKey = _imp->isNearbyKeyFrame(e->pos(), &selectedKeyCurve, &selectedKey, &selectedKeyHasPrev, &selectedKeyPrev,
                                                 &selectedKeyHasNext, &selectedKeyNext);
    if (hasSelectedKey) {
        _imp->_drawSelectedKeyFramesBbox = false;
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingKeys;
        setCursor( QCursor(Qt::CrossCursor) );

        if ( !modCASIsControl(e) ) {
            _imp->_selectedKeyFrames.clear();
        }
        KeyPtr selected ( new SelectedKey(selectedKeyCurve, selectedKey, selectedKeyHasPrev, selectedKeyPrev,
                                          selectedKeyHasNext, selectedKeyNext) );

        _imp->refreshKeyTangents(selected);

        //insert it into the _selectedKeyFrames
        _imp->insertSelectedKeyFrameConditionnaly(selected);

        _imp->_keyDragLastMovement.rx() = 0.;
        _imp->_keyDragLastMovement.ry() = 0.;
        _imp->_dragStartPoint = e->pos();
        _imp->_lastMousePos = e->pos();
        update(); // the keyframe changes color and the derivatives must be drawn
        e->accept();

        return;
    }


    ////
    // is the click near a derivative manipulator?
    std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> selectedTan = _imp->isNearbyTangent( e->pos() );

    //select the derivative only if it is not a constant keyframe
    if ( selectedTan.second && (selectedTan.second->key.getInterpolation() != eKeyframeTypeConstant) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingTangent;
        _imp->_selectedDerivative = selectedTan;
        _imp->_lastMousePos = e->pos();
        //no need to set _imp->_dragStartPoint
        update();
        e->accept();

        return;
    }

    KeyPtr nearbyKeyText = _imp->isNearbyKeyFrameText( e->pos() );
    if (nearbyKeyText) {
        // do nothing, doubleclick edits the text
        e->accept();

        return;
    }

    std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> tangentText = _imp->isNearbySelectedTangentText( e->pos() );
    if (tangentText.second) {
        // do nothing, doubleclick edits the text
        e->accept();

        return;
    }


    ////
    // is the click near the vertical current time marker?
    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingTimeline;
        _imp->_lastMousePos = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to update()
        e->accept();

        return;
    }

    // yes, select it and don't start any other action, the user can then do per-curve specific actions
    // like centering on it on the viewport or pasting previously copied keyframes.
    // This is kind of the last resort action before the default behaviour (which is to draw
    // a selection rectangle), because we'd rather select a keyframe than the nearby curve
    {
        double xCurve, yCurve;
        Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
        if ( foundCurveNearby != _imp->_curves.end() ) {
            _imp->selectCurve(*foundCurveNearby);
        }
    }

    ////
    // default behaviour: unselect selected keyframes, if any, and start a new selection
    _imp->_drawSelectedKeyFramesBbox = false;
    if ( !modCASIsControl(e) ) {
        _imp->_selectedKeyFrames.clear();
    }
    _imp->_state = eEventStateSelecting;
    _imp->_lastMousePos = e->pos();
    _imp->_dragStartPoint = e->pos();
    update();
    e->accept();
} // mousePressEvent

void
CurveWidget::mouseReleaseEvent(QMouseEvent*)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (_imp->_evaluateOnPenUp) {
        _imp->_evaluateOnPenUp = false;

        if ( (_imp->_state == eEventStateDraggingKeys) ||
             ( _imp->_state == eEventStateDraggingBtmLeftBbox) ||
             ( _imp->_state == eEventStateDraggingMidBtmBbox) ||
             ( _imp->_state == eEventStateDraggingBtmRightBbox) ||
             ( _imp->_state == eEventStateDraggingMidRightBbox) ||
             ( _imp->_state == eEventStateDraggingTopRightBbox) ||
             ( _imp->_state == eEventStateDraggingMidTopBbox) ||
             ( _imp->_state == eEventStateDraggingTopLeftBbox) ||
             ( _imp->_state == eEventStateDraggingMidLeftBbox) ) {
            if (_imp->_gui) {
                _imp->_gui->setDraftRenderEnabled(false);
            }

            std::map<KnobHolder*, bool> toEvaluate;
            std::list<RotoContextPtr> rotoToEvaluate;
            for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
                KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( it->first.get() );
                BezierCPCurveGui* isBezierCurve = dynamic_cast<BezierCPCurveGui*>( it->first.get() );
                if (isKnobCurve) {
                    if ( !isKnobCurve->getKnobGui() ) {
                        RotoContextPtr roto = isKnobCurve->getRotoContext();
                        assert(roto);
                        rotoToEvaluate.push_back(roto);
                    } else {
                        KnobIPtr knob = isKnobCurve->getInternalKnob();
                        assert(knob);
                        KnobHolder* holder = knob->getHolder();
                        assert(holder);
                        std::map<KnobHolder*, bool>::iterator found = toEvaluate.find(holder);
                        bool evaluateOnChange = knob->getEvaluateOnChange();
                        if ( ( found != toEvaluate.end() ) && !found->second && evaluateOnChange ) {
                            found->second = true;
                        } else if ( found == toEvaluate.end() ) {
                            toEvaluate.insert( std::make_pair(holder, evaluateOnChange) );
                        }
                    }
                } else if (isBezierCurve) {
                    rotoToEvaluate.push_back( isBezierCurve->getRotoContext() );
                }
            }
            for (std::map<KnobHolder*, bool>::iterator it = toEvaluate.begin(); it != toEvaluate.end(); ++it) {
                it->first->incrHashAndEvaluate(it->second, false);
            }
            for (std::list<RotoContextPtr>::iterator it = rotoToEvaluate.begin(); it != rotoToEvaluate.end(); ++it) {
                (*it)->evaluateChange();
            }
        } else if (_imp->_state == eEventStateDraggingTangent) {
            if (_imp->_gui) {
                _imp->_gui->setDraftRenderEnabled(false);
            }

            KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( _imp->_selectedDerivative.second->curve.get() );
            BezierCPCurveGui* isBezierCurve = dynamic_cast<BezierCPCurveGui*>( _imp->_selectedDerivative.second->curve.get() );
            if (isKnobCurve) {
                if ( !isKnobCurve->getKnobGui() ) {
                    RotoContextPtr roto = isKnobCurve->getRotoContext();
                    assert(roto);
                    roto->evaluateChange();
                } else {
                    KnobIPtr toEvaluate = isKnobCurve->getInternalKnob();
                    assert(toEvaluate);
                    toEvaluate->getHolder()->incrHashAndEvaluate(true, false);
                }
            } else if (isBezierCurve) {
                isBezierCurve->getRotoContext()->evaluateChange();
            }
        }
    }

    EventStateEnum prevState = _imp->_state;
    _imp->_state = eEventStateNone;
    _imp->_selectionRectangle.setBottomRight( QPointF(0, 0) );
    _imp->_selectionRectangle.setTopLeft( _imp->_selectionRectangle.bottomRight() );
    if ( !_imp->_selectedKeyFrames.empty() && ( (_imp->_selectedKeyFrames.size() > 1) || (_imp->_selectedKeyFrames.begin()->second.size() > 1) ) ) {
        _imp->_drawSelectedKeyFramesBbox = true;
    }
    if (prevState == eEventStateDraggingTimeline) {
        if ( _imp->_gui->isDraftRenderEnabled() ) {
            _imp->_gui->setDraftRenderEnabled(false);
            bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
            if (autoProxyEnabled) {
                _imp->_gui->renderAllViewers(true);
            }
        }
    }

    if (prevState == eEventStateSelecting) { // should other cases be considered?
        update();
    }
} // CurveWidget::mouseReleaseEvent

void
CurveWidget::mouseMoveEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //setFocus();

    //set cursor depending on the situation

    //find out if there is a nearby  derivative handle
    std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> selectedTan = _imp->isNearbyTangent( e->pos() );

    //if the selected keyframes rectangle is drawn and we're nearby the cross
    if ( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
        setCursor( QCursor(Qt::SizeAllCursor) );
    } else {
        //if there's a keyframe handle nearby

        CurveGuiPtr selectedKeyCurve;
        KeyFrame selectedKey, selectedKeyPrev, selectedKeyNext;
        bool selectedKeyHasPrev, selectedKeyHasNext;
        bool hasSelectedKey = _imp->isNearbyKeyFrame(e->pos(), &selectedKeyCurve, &selectedKey, &selectedKeyHasPrev, &selectedKeyPrev,
                                                     &selectedKeyHasNext, &selectedKeyNext);

        //if there's a keyframe or derivative handle nearby set the cursor to cross
        if (hasSelectedKey || selectedTan.second) {
            setCursor( QCursor(Qt::CrossCursor) );
        } else {
            KeyPtr keyframeText = _imp->isNearbyKeyFrameText( e->pos() );
            if (keyframeText) {
                setCursor( QCursor(Qt::IBeamCursor) );
            } else {
                std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> tangentText = _imp->isNearbySelectedTangentText( e->pos() );
                if (tangentText.second) {
                    setCursor( QCursor(Qt::IBeamCursor) );
                } else {
                    //if we're nearby a timeline polygon, set cursor to horizontal displacement
                    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
                        setCursor( QCursor(Qt::SizeHorCursor) );
                    } else {
                        //default case
                        unsetCursor();
                    }
                }
            }
        }
    }

    if (_imp->_state == eEventStateNone) {
        // nothing else to do
        QGLWidget::mouseMoveEvent(e);

        return;
    }

    bool mustUpdate = true;

    // after this point , only mouse dragging situations are handled
    assert(_imp->_state != eEventStateNone);

    if (_imp->_mustSetDragOrientation) {
        QPointF diff(e->pos() - _imp->_dragStartPoint);
        double dist = diff.manhattanLength();
        if (dist > 5) {
            if ( std::abs( diff.x() ) > std::abs( diff.y() ) ) {
                _imp->_mouseDragOrientation.setX(1);
                _imp->_mouseDragOrientation.setY(0);
            } else {
                _imp->_mouseDragOrientation.setX(0);
                _imp->_mouseDragOrientation.setY(1);
            }
            _imp->_mustSetDragOrientation = false;
        }
    }

    QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->_lastMousePos.x(), _imp->_lastMousePos.y() );
    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );
    switch (_imp->_state) {
    case eEventStateDraggingView:
        _imp->zoomOrPannedSinceLastFit = true;
        _imp->zoomCtx.translate(dx, dy);

        // Synchronize the dope sheet editor and opened viewers
        if ( _imp->_gui->isTripleSyncEnabled() ) {
            _imp->updateDopeSheetViewFrameRange();
            _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
        }
        break;

    case eEventStateDraggingKeys:
        if (!_imp->_mustSetDragOrientation) {
            if ( !_imp->_selectedKeyFrames.empty() ) {
                if (_imp->_gui) {
                    _imp->_gui->setDraftRenderEnabled(true);
                }
                _imp->moveSelectedKeyFrames(oldClick_opengl, newClick_opengl);
            }
        }
        break;
    case eEventStateDraggingBtmLeftBbox:
    case eEventStateDraggingMidBtmBbox:
    case eEventStateDraggingBtmRightBbox:
    case eEventStateDraggingMidRightBbox:
    case eEventStateDraggingTopRightBbox:
    case eEventStateDraggingMidTopBbox:
    case eEventStateDraggingTopLeftBbox:
    case eEventStateDraggingMidLeftBbox:
        if ( !_imp->_selectedKeyFrames.empty() ) {
            if (_imp->_gui) {
                _imp->_gui->setDraftRenderEnabled(true);
            }
            _imp->transformSelectedKeyFrames( oldClick_opengl, newClick_opengl, modCASIsShift(e) );
        }
        break;
    case eEventStateSelecting:
        _imp->refreshSelectionRectangle( (double)e->x(), (double)e->y() );
        break;

    case eEventStateDraggingTangent:
        if (_imp->_gui) {
            _imp->_gui->setDraftRenderEnabled(true);
        }
        _imp->moveSelectedTangent(newClick_opengl);
        break;

    case eEventStateDraggingTimeline:
        _imp->_gui->setDraftRenderEnabled(true);
        _imp->_gui->getApp()->setLastViewerUsingTimeline( NodePtr() );
        _imp->_timeline->seekFrame( (SequenceTime)newClick_opengl.x(), false, 0,  eTimelineChangeReasonCurveEditorSeek );
        break;
    case eEventStateZooming: {
        if ( (_imp->zoomCtx.screenWidth() > 0) && (_imp->zoomCtx.screenHeight() > 0) ) {
            _imp->zoomOrPannedSinceLastFit = true;

            int deltaX = 2 * ( e->x() - _imp->_lastMousePos.x() );
            int deltaY = -2 * ( e->y() - _imp->_lastMousePos.y() );
            // Wheel: zoom values and time, keep point under mouse
            double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX);
            double scaleFactorY = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaY);
            QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( _imp->_dragStartPoint.x(), _imp->_dragStartPoint.y() );

            if ( modCASIsControlShift(e) ) {
                // Alt + Shift + Wheel: zoom values only, keep point under mouse
                _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactorY);
            } else if ( modCASIsControl(e) ) {
                // Alt + Wheel: zoom time only, keep point under mouse
                _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactorX);
            }
            if (_imp->_drawSelectedKeyFramesBbox) {
                refreshSelectedKeysBbox();
            }

            // Synchronize the dope sheet editor and opened viewers
            if ( _imp->_gui->isTripleSyncEnabled() ) {
                _imp->updateDopeSheetViewFrameRange();
                _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
            }
            refreshDisplayedTangents();
        }
        break;
    }
    case eEventStateNone:
        assert(0);
        break;
    } // switch

    _imp->_lastMousePos = e->pos();

    if (mustUpdate) {
        update();
    }
    QGLWidget::mouseMoveEvent(e);
} // mouseMoveEvent

void
CurveWidget::refreshSelectedKeysBbox()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( (_imp->zoomCtx.screenWidth() < 1) || (_imp->zoomCtx.screenHeight() < 1) ) {
        return;
    }

    RectD keyFramesBbox;
    bool bboxSet = false;
    for (SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin();
         it != _imp->_selectedKeyFrames.end();
         ++it) {
        for (std::list<KeyPtr>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            double x = (*it2)->key.getTime();
            double y = (*it2)->key.getValue();
            if (bboxSet) {
                if ( x < keyFramesBbox.left() ) {
                    keyFramesBbox.set_left(x);
                }
                if ( x > keyFramesBbox.right() ) {
                    keyFramesBbox.set_right(x);
                }
                if ( y > keyFramesBbox.top() ) {
                    keyFramesBbox.set_top(y);
                }
                if ( y < keyFramesBbox.bottom() ) {
                    keyFramesBbox.set_bottom(y);
                }
            } else {
                bboxSet = true;
                keyFramesBbox.set_left(x);
                keyFramesBbox.set_right(x);
                keyFramesBbox.set_top(y);
                keyFramesBbox.set_bottom(y);
            }
        }
    }
    QPointF topLeft( keyFramesBbox.left(), keyFramesBbox.top() );
    QPointF btmRight( keyFramesBbox.right(), keyFramesBbox.bottom() );
    _imp->_selectedKeyFramesBbox.setTopLeft(topLeft);
    _imp->_selectedKeyFramesBbox.setBottomRight(btmRight);

    QPointF middle( ( topLeft.x() + btmRight.x() ) / 2., ( topLeft.y() + btmRight.y() ) / 2. );
    QPointF middleWidgetCoord = toWidgetCoordinates( middle.x(), middle.y() );
    QPointF middleLeft = _imp->zoomCtx.toZoomCoordinates( middleWidgetCoord.x() - 20, middleWidgetCoord.y() );
    QPointF middleRight = _imp->zoomCtx.toZoomCoordinates( middleWidgetCoord.x() + 20, middleWidgetCoord.y() );
    QPointF middleTop = _imp->zoomCtx.toZoomCoordinates(middleWidgetCoord.x(), middleWidgetCoord.y() - 20);
    QPointF middleBottom = _imp->zoomCtx.toZoomCoordinates(middleWidgetCoord.x(), middleWidgetCoord.y() + 20);

    _imp->_selectedKeyFramesCrossHorizLine.setPoints(middleLeft, middleRight);
    _imp->_selectedKeyFramesCrossVertLine.setPoints(middleBottom, middleTop);
} // CurveWidget::refreshSelectedKeysBbox

void
CurveWidget::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( modCASIsControlShift(e) ) {
        _imp->zoomOrPannedSinceLastFit = true;
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else if ( modCASIsControl(e) ) {
        _imp->zoomOrPannedSinceLastFit = true;
        // Alt + Wheel: zoom time only, keep point under mouse
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else {
        _imp->zoomOrPannedSinceLastFit = true;
        // Wheel: zoom values and time, keep point under mouse
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }

    if (_imp->_drawSelectedKeyFramesBbox) {
        refreshSelectedKeysBbox();
    }


    // Synchronize the dope sheet editor and opened viewers
    if ( _imp->_gui->isTripleSyncEnabled() ) {
        _imp->updateDopeSheetViewFrameRange();
        _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
    }
    refreshDisplayedTangents();
    update();
} // wheelEvent

QPointF
CurveWidget::toZoomCoordinates(double x,
                               double y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->zoomCtx.toZoomCoordinates(x, y);
}

QPointF
CurveWidget::toWidgetCoordinates(double x,
                                 double y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->zoomCtx.toWidgetCoordinates(x, y);
}

/**
 * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
 **/
void
CurveWidget::toWidgetCoordinates(double *x,
                                 double *y) const
{
    QPointF p = _imp->zoomCtx.toWidgetCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

/**
 * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
 **/
void
CurveWidget::toCanonicalCoordinates(double *x,
                                    double *y) const
{
    QPointF p = _imp->zoomCtx.toZoomCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

/**
 * @brief Returns the font height, i.e: the height of the highest letter for this font
 **/
int
CurveWidget::getWidgetFontHeight() const
{
    return fontMetrics().height();
}

/**
 * @brief Returns for a string the estimated pixel size it would take on the widget
 **/
int
CurveWidget::getStringWidthForCurrentFont(const std::string& string) const
{
    return fontMetrics().width( QString::fromUtf8( string.c_str() ) );
}

QSize
CurveWidget::sizeHint() const
{
    return _imp->sizeH;
}

void
CurveWidget::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool accept = true;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys, modifiers, key) ) {
        deleteSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorConstant, modifiers, key) ) {
        constantInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorLinear, modifiers, key) ) {
        linearInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSmooth, modifiers, key) ) {
        smoothForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCatmullrom, modifiers, key) ) {
        catmullromInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCubic, modifiers, key) ) {
        cubicInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorHorizontal, modifiers, key) ) {
        horizontalInterpForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorBreak, modifiers, key) ) {
        breakDerivativesForSelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenterAll, modifiers, key) ) {
        frameAll();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenter, modifiers, key) ) {
        frameSelectedCurve();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll, modifiers, key) ) {
        selectAllKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCopy, modifiers, key) ) {
        copySelectedKeyFramesToClipBoard();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorPaste, modifiers, key) ) {
        pasteKeyFramesFromClipBoardToSelectedCurve();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomIn, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomOut, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else {
        accept = false;
    }

    CurveEditor* ce = 0;
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if ( parent->objectName() == QString::fromUtf8("CurveEditor") ) {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }

    if (accept) {
        if (ce) {
            ce->onInputEventCalled();
        }

        e->accept();
    } else {
        if (ce) {
            ce->handleUnCaughtKeyPressEvent(e);
        }
        QGLWidget::keyPressEvent(e);
    }
} // keyPressEvent

void
CurveWidget::enterEvent(QEvent* e)
{
    setFocus();
    QGLWidget::enterEvent(e);
}

void
CurveWidget::refreshDisplayedTangents()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        for (std::list<KeyPtr>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            _imp->refreshKeyTangents(*it2);
        }
    }
}

void
CurveWidget::refreshCurveDisplayTangents(CurveGui* curve)
{
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        if (it->first.get() == curve) {
            for (std::list<KeyPtr>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                _imp->refreshKeyTangents(*it2);
            }

            break;
        }
    }
}

void
CurveWidget::setSelectedKeys(const SelectedKeys & keys)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_selectedKeyFrames = keys;
    refreshSelectedKeysAndUpdate();
}

void
CurveWidget::refreshSelectedKeysAndUpdate()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    refreshSelectedKeysBbox();

    refreshDisplayedTangents();
    update();
}

void
CurveWidget::constantInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeConstant);
}

void
CurveWidget::linearInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeLinear);
}

void
CurveWidget::smoothForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
}

void
CurveWidget::catmullromInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
}

void
CurveWidget::cubicInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeCubic);
}

void
CurveWidget::horizontalInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
}

void
CurveWidget::breakDerivativesForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(eKeyframeTypeBroken);
}

void
CurveWidget::deleteSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( _imp->_selectedKeyFrames.empty() ) {
        return;
    }

    _imp->_drawSelectedKeyFramesBbox = false;
    _imp->_selectedKeyFramesBbox.setBottomRight( QPointF(0, 0) );
    _imp->_selectedKeyFramesBbox.setTopLeft( _imp->_selectedKeyFramesBbox.bottomRight() );

    //apply the same strategy than for moveSelectedKeyFrames()

    std::map<CurveGuiPtr, std::vector<KeyFrame> >  toRemove;
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        std::vector<KeyFrame>& vect = toRemove[it->first];
        for (std::list<KeyPtr>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            vect.push_back( (*it2)->key );
        }
    }

    pushUndoCommand( new RemoveKeysCommand(this, toRemove) );


    _imp->_selectedKeyFrames.clear();

    update();
}

void
CurveWidget::copySelectedKeyFramesToClipBoard()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_keyFramesClipBoard.clear();
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        for (std::list<KeyPtr>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            _imp->_keyFramesClipBoard.push_back( (*it2)->key );
        }
    }
}

void
CurveWidget::pasteKeyFramesFromClipBoardToSelectedCurve()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    CurveGuiPtr curve;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isSelected() ) {
            curve = (*it);
            break;
        }
    }
    if (!curve) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select a curve first.").toStdString() );

        return;
    }
    //this function will call update() for us
    pushUndoCommand( new AddKeysCommand(this, curve, _imp->_keyFramesClipBoard) );
}

void
CurveWidget::selectAllKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_drawSelectedKeyFramesBbox = true;
    _imp->_selectedKeyFrames.clear();
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {

            CurvePtr internalCurve = (*it)->getInternalCurve();
            bool isPeriodic = false;
            std::pair<double,double> parametricRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
            if (internalCurve) {
                isPeriodic = internalCurve->isCurvePeriodic();
                parametricRange = internalCurve->getXRange();
            }
            KeyFrameSet set = (*it)->getKeyFrames();
            std::list<KeyPtr>& selectedKeysForcurve = _imp->_selectedKeyFrames[*it];
            KeyFrameSet::const_iterator it2 = set.begin();
            KeyFrameSet::const_iterator prev = set.end();
            KeyFrameSet::const_iterator next = it2;
            ++next;
            for (; it2 != set.end(); ++it2) {
                KeyFrame prevKey;
                bool hasPrev = false;
                if ( prev != set.end() ) {
                    prevKey = *prev;
                    hasPrev = true;
                } else if (isPeriodic) {
                    KeyFrameSet::const_reverse_iterator last = set.rbegin();
                    prevKey = *last;
                    prevKey.setTime(prevKey.getTime() - (parametricRange.second - parametricRange.first));
                    hasPrev = true;
                }
                KeyFrame nextKey;
                bool hasNext = false;
                if ( next != set.end() ) {
                    nextKey = *next;
                    hasNext = true;
                } else if (isPeriodic) {
                    KeyFrameSet::const_iterator start = set.begin();
                    nextKey = *start;
                    nextKey.setTime(nextKey.getTime() + (parametricRange.second - parametricRange.first));
                    hasNext = true;
                }
                KeyPtr newSelectedKey( new SelectedKey(*it, *it2, hasPrev, prevKey, hasNext, nextKey) );
                selectedKeysForcurve.push_back(newSelectedKey);

                if ( prev != set.end() ) {
                    ++prev;
                } else {
                    prev = set.begin();
                }
                if ( next != set.end() ) {
                    ++next;
                }
            }
        }
    }

    refreshSelectedKeysAndUpdate();
}

void
CurveWidget::loopSelectedCurve()
{
    CurveEditor* ce = 0;

    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if ( parent->objectName() == QString::fromUtf8("CurveEditor") ) {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (!ce) {
        return;
    }

    CurveGuiPtr curve = ce->getSelectedCurve();
    if (!curve) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select a curve first in the view.").toStdString() );

        return;
    }
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>( curve.get() );
    assert(knobCurve);
    if (!knobCurve) {
        throw std::logic_error("CurveWidget::loopSelectedCurve");
    }
    NATRON_PYTHON_NAMESPACE::PyModalDialog dialog(_imp->_gui);
    NATRON_PYTHON_NAMESPACE::IntParamPtr firstFrame( dialog.createIntParam( QString::fromUtf8("firstFrame"), QString::fromUtf8("First frame") ) );
    firstFrame->setAnimationEnabled(false);
    NATRON_PYTHON_NAMESPACE::IntParamPtr lastFrame( dialog.createIntParam( QString::fromUtf8("lastFrame"), QString::fromUtf8("Last frame") ) );
    lastFrame->setAnimationEnabled(false);
    dialog.refreshUserParamsGUI();
    if ( dialog.exec() ) {
        int first = firstFrame->getValue();
        int last = lastFrame->getValue();
        std::stringstream ss;
        ss << "curve(((frame - " << first << ") % (" << last << " - " << first << " + 1)) + " << first << ", " << knobCurve->getDimension() << ")";
        std::string script = ss.str();
        ce->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ) );
    }
}

void
CurveWidget::negateSelectedCurve()
{
    CurveEditor* ce = 0;

    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if ( parent->objectName() == QString::fromUtf8("CurveEditor") ) {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (!ce) {
        return;
    }
    CurveGuiPtr curve = ce->getSelectedCurve();
    if (!curve) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select a curve first in the view.").toStdString() );

        return;
    }
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>( curve.get() );
    assert(knobCurve);
    if (!knobCurve) {
        throw std::logic_error("CurveWidget::negateSelectedCurve");
    }
    std::stringstream ss;
    ss << "-curve(frame, " << knobCurve->getDimension() << ")";
    std::string script = ss.str();
    ce->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ) );
}

void
CurveWidget::reverseSelectedCurve()
{
    CurveEditor* ce = 0;

    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if ( parent->objectName() == QString::fromUtf8("CurveEditor") ) {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    if (!ce) {
        return;
    }
    CurveGuiPtr curve = ce->getSelectedCurve();
    if (!curve) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select a curve first in the view.").toStdString() );

        return;
    }
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>( curve.get() );
    assert(knobCurve);
    if (!knobCurve) {
        throw std::logic_error("CurveWidget::reverseSelectedCurve");
    }
    std::stringstream ss;
    ss << "curve(-frame, " << knobCurve->getDimension() << ")";
    std::string script = ss.str();
    ce->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ) );
}

void
CurveWidget::frameAll()
{
    centerOn(std::vector<CurveGuiPtr>(), false);
}

void
CurveWidget::frameSelectedCurve()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<CurveGuiPtr> selection;
    _imp->_selectionModel->getSelectedCurves(&selection);
    if ( selection.empty() ) {
        frameAll();
        //Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select a curve first in the left pane.").toStdString() );
    } else {
        centerOn(selection, false);
    }
}

void
CurveWidget::onTimeLineFrameChanged(SequenceTime,
                                    int /*reason*/)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( !_imp->_gui || _imp->_gui->isGUIFrozen() ) {
        return;
    }

    if (!_imp->_timelineEnabled) {
        _imp->_timelineEnabled = true;
    }
    _imp->refreshTimelinePositions();
    if ( isVisible() ) {
        update();
    }
}

void
CurveWidget::onTimeLineBoundariesChanged(int,
                                         int)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    update();
}

const QColor &
CurveWidget::getSelectedCurveColor() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->_selectedCurveColor;
}

const QFont &
CurveWidget::getFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return *_imp->_font;
}

const SelectedKeys &
CurveWidget::getSelectedKeyFrames() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->_selectedKeyFrames;
}

const QFont &
CurveWidget::getTextFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return *_imp->_font;
}

void
CurveWidget::centerOn(double xmin,
                      double xmax)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( (_imp->zoomCtx.screenWidth() > 0) && (_imp->zoomCtx.screenHeight() > 0) ) {
        _imp->zoomCtx.fill( xmin, xmax, _imp->zoomCtx.bottom(), _imp->zoomCtx.top() );
    }

    update();
}

void
CurveWidget::getProjection(double *zoomLeft,
                           double *zoomBottom,
                           double *zoomFactor,
                           double *zoomAspectRatio) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomAspectRatio = _imp->zoomCtx.aspectRatio();
}

void
CurveWidget::setProjection(double zoomLeft,
                           double zoomBottom,
                           double zoomFactor,
                           double zoomAspectRatio)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
}

void
CurveWidget::onUpdateOnPenUpActionTriggered()
{
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    appPTR->getCurrentSettings()->setRenderOnEditingFinishedOnly(!updateOnPenUpOnly);
}

void
CurveWidget::focusInEvent(QFocusEvent* e)
{
    QGLWidget::focusInEvent(e);
}

void
CurveWidget::exportCurveToAscii()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<CurveGuiPtr> curves;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( it->get() );
        if ( (*it)->isVisible() && isKnobCurve ) {
            KnobIPtr knob = isKnobCurve->getInternalKnob();
            KnobStringBase* isString = dynamic_cast<KnobStringBase*>( knob.get() );
            if (isString) {
                Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("String curves cannot be imported/exported.").toStdString() );

                return;
            }
            curves.push_back(*it);
        }
    }
    if ( curves.empty() ) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must have a curve on the editor first.").toStdString() );

        return;
    }

    ImportExportCurveDialog dialog(true, curves, _imp->_gui, this);
    if ( dialog.exec() ) {
        double xstart = dialog.getXStart();
        int count = dialog.getXCount();
        double incr = dialog.getXIncrement();
        std::map<int, CurveGuiPtr> columns;
        dialog.getCurveColumns(&columns);

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            //double end = xstart + (count-1) * incr;
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( (int)incr != incr) || ( (int)xstart != xstart ) ) ) {
                Dialogs::warningDialog( tr("Curve Export").toStdString(), tr("%1 doesn't support X values that are not integers.").arg( curves[i]->getName() ).toStdString() );

                return;
            }
        }

        assert( !columns.empty() );
        int columnsCount = columns.rbegin()->first + 1;

        ///setup the file
        QString name = dialog.getFilePath();
        QFile file(name);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream ts(&file);

        for (int i = 0; i < count; ++i) {
            for (int c = 0; c < columnsCount; ++c) {
                std::map<int, CurveGuiPtr>::const_iterator foundCurve = columns.find(c);
                if ( foundCurve != columns.end() ) {
                    QString str = QString::number(foundCurve->second->evaluate(true, xstart + i * incr), 'f', 10);
                    ts << str;
                } else {
                    ts <<  0;
                }
                if (c < columnsCount - 1) {
                    ts << ' ';
                }
            }
            ts << '\n';
        }


        ///close the file
        file.close();
    }
} // exportCurveToAscii

void
CurveWidget::importCurveFromAscii()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<CurveGuiPtr> curves;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( it->get() );
        if ( (*it)->isVisible() && isKnobCurve ) {
            KnobIPtr knob = isKnobCurve->getInternalKnob();
            KnobStringBase* isString = dynamic_cast<KnobStringBase*>( knob.get() );
            if (isString) {
                Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("String curves cannot be imported/exported.").toStdString() );

                return;
            }

            curves.push_back(*it);
        }
    }
    if ( curves.empty() ) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must have a curve on the editor first.").toStdString() );

        return;
    }

    ImportExportCurveDialog dialog(false, curves, _imp->_gui, this);
    if ( dialog.exec() ) {
        QString filePath = dialog.getFilePath();
        if ( !QFile::exists(filePath) ) {
            Dialogs::warningDialog( tr("Curve Import").toStdString(), tr("File not found.").toStdString() );

            return;
        }

        double x = dialog.getXStart();
        double incr = dialog.getXIncrement();
        std::map<int, CurveGuiPtr> columns;
        dialog.getCurveColumns(&columns);
        assert( !columns.empty() );

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) ) ) {
                Dialogs::warningDialog( tr("Curve Import").toStdString(), tr("%1 doesn't support X values that are not integers.").arg( curves[i]->getName() ).toStdString() );

                return;
            }
        }

        QFile file( dialog.getFilePath() );
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        std::map<CurveGuiPtr, std::vector<double> > curvesValues;
        ///scan the file to get the curve values
        while ( !ts.atEnd() ) {
            QString line = ts.readLine();
            if ( line.isEmpty() ) {
                continue;
            }
            int i = 0;
            std::vector<double> values;

            ///read the line to extract all values
            while ( i < line.size() ) {
                QString value;
                while ( i < line.size() && line.at(i) != QLatin1Char('_') ) {
                    value.push_back( line.at(i) );
                    ++i;
                }
                if ( i < line.size() ) {
                    if ( line.at(i) != QLatin1Char('_') ) {
                        Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("The file could not be read.").toStdString() );

                        return;
                    }
                    ++i;
                }
                bool ok;
                double v = value.toDouble(&ok);
                if (!ok) {
                    Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("The file could not be read.").toStdString() );

                    return;
                }
                values.push_back(v);
            }
            ///assert that the values count is greater than the number of curves provided by the user
            if ( values.size() < columns.size() ) {
                Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("The file contains less curves than what you selected.").toStdString() );

                return;
            }

            for (std::map<int, CurveGuiPtr>::const_iterator col = columns.begin(); col != columns.end(); ++col) {
                if ( col->first >= (int)values.size() ) {
                    Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("One of the curve column index is not a valid index for the given file.").toStdString() );

                    return;
                }
                std::map<CurveGuiPtr, std::vector<double> >::iterator foundCurve = curvesValues.find(col->second);
                if ( foundCurve != curvesValues.end() ) {
                    foundCurve->second.push_back(values[col->first]);
                } else {
                    std::vector<double> curveValues(1);
                    curveValues[0] = values[col->first];
                    curvesValues.insert( std::make_pair(col->second, curveValues) );
                }
            }
        }
        ///now restore the curves since we know what we read is valid
        for (std::map<CurveGuiPtr, std::vector<double> >::const_iterator it = curvesValues.begin(); it != curvesValues.end(); ++it) {
            std::vector<KeyFrame> keys;
            const std::vector<double> & values = it->second;
            double xIndex = x;
            for (U32 i = 0; i < values.size(); ++i) {
                KeyFrame k(xIndex, values[i], 0., 0., eKeyframeTypeLinear);
                keys.push_back(k);
                xIndex += incr;
            }

            pushUndoCommand( new SetKeysCommand(this, it->first, keys) );
        }
        _imp->_selectedKeyFrames.clear();
        update();
    }
} // importCurveFromAscii

void
CurveWidget::setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc)
{
    _imp->_customInteract = interactDesc;
}

OfxParamOverlayInteractPtr
CurveWidget::getCustomInteract() const
{
    return _imp->_customInteract.lock();
}

void
CurveWidget::addKey(const CurveGuiPtr& curve, double xCurve, double yCurve)
{
    _imp->selectCurve(curve);

    Curve::YRange yRange = curve->getCurveYRange();
    if ( (yCurve < yRange.min) || (yCurve > yRange.max) ) {
        QString err =  tr("Out of curve y range ") +
        QString::fromUtf8("[%1 - %2]").arg(yRange.min).arg(yRange.max);
        Dialogs::warningDialog( "", err.toStdString() );

        return;
    }
    std::vector<KeyFrame> keys(1);
    keys[0] = KeyFrame(xCurve, yCurve, 0, 0);
    pushUndoCommand( new AddKeysCommand(this, curve, keys) );

    _imp->_drawSelectedKeyFramesBbox = false;
    _imp->_mustSetDragOrientation = true;
    _imp->_state = eEventStateDraggingKeys;
    setCursor( QCursor(Qt::CrossCursor) );

    _imp->_selectedKeyFrames.clear();

    KeyFrameSet keySet = curve->getKeyFrames();
    KeyFrameSet::const_iterator foundKey = Curve::findWithTime(keySet, xCurve);
    assert( foundKey != keySet.end() );

    CurvePtr internalCurve = curve->getInternalCurve();
    bool isPeriodic = false;
    std::pair<double,double> parametricRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    if (internalCurve) {
        isPeriodic = internalCurve->isCurvePeriodic();
        parametricRange = internalCurve->getXRange();
    }


    KeyFrame prevKey, nextKey;
    bool hasPrev = foundKey != keySet.begin();
    if (hasPrev) {
        KeyFrameSet::const_iterator prevIt = foundKey;
        --prevIt;
        prevKey = *prevIt;
    } else if (isPeriodic) {
        KeyFrameSet::const_reverse_iterator last = keySet.rbegin();
        prevKey = *last;
        prevKey.setTime(prevKey.getTime() - (parametricRange.second - parametricRange.first));
        hasPrev = true;
    }
    KeyFrameSet::const_iterator next = foundKey;
    ++next;
    bool hasNext = next != keySet.end();
    if (hasNext) {
        nextKey = *next;
    } else if (isPeriodic) {
        KeyFrameSet::const_iterator start = keySet.begin();
        nextKey = *start;
        nextKey.setTime(nextKey.getTime() + (parametricRange.second - parametricRange.first));
    }

    KeyPtr selected( new SelectedKey(curve, *foundKey, hasPrev, prevKey, hasNext, nextKey) );

    _imp->refreshKeyTangents(selected);

    //insert it into the _selectedKeyFrames
    _imp->insertSelectedKeyFrameConditionnaly(selected);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_CurveWidget.cpp"

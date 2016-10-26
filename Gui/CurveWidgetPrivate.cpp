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

#include "CurveWidgetPrivate.h"

#include <cmath> // floor
#include <algorithm> // min, max
#include <stdexcept>

#include <QtCore/QThread>
#include <QApplication>
#include <QtCore/QDebug>

#include "Engine/Bezier.h"
#include "Engine/TimeLine.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h" // Image::clamp
#include "Engine/StringAnimationManager.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Menu.h"
#include "Gui/ticks.h"

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
// (in widget pixels)
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8


#define BOUNDING_BOX_HANDLE_SIZE 4

#define AXIS_MAX 100000.
#define AXIS_MIN -100000.

NATRON_NAMESPACE_ENTER;


CurveWidgetPrivate::CurveWidgetPrivate(Gui* gui,
                                       const AnimationModuleBasePtr& model,
                                       CurveWidget* publicInterface)
    : _model(model)
    , _lastMousePos()
    , zoomCtx()
    , _state(eEventStateNone)
    , _rightClickMenu( new Menu(publicInterface) )
    , textRenderer()
    , _mustSetDragOrientation(false)
    , _mouseDragOrientation()
    , _keyFramesClipBoard()
    , _selectionRectangle()
    , _dragStartPoint()
    , _selectedKeyFramesBbox()
    , _selectedKeyFramesCrossVertLine()
    , _selectedKeyFramesCrossHorizLine()
    , _selectedDerivative()
    , _evaluateOnPenUp(false)
    , _keyDragLastMovement()
    , _gui(gui)
    , savedTexture(0)
    , sizeH()
    , zoomOrPannedSinceLastFit(false)
    , drawnOnce(false)
    , _widget(publicInterface)
{
}

CurveWidgetPrivate::~CurveWidgetPrivate()
{

}

void
CurveWidgetPrivate::createMenu()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _rightClickMenu->clear();

    Menu* fileMenu = new Menu(_rightClickMenu);
    //fileMenu->setFont( QFont(appFont,appFontSize) );
    fileMenu->setTitle( tr("File") );
    _rightClickMenu->addAction( fileMenu->menuAction() );

    Menu* editMenu = new Menu(_rightClickMenu);
    //editMenu->setFont( QFont(appFont,appFontSize) );
    editMenu->setTitle( tr("Edit") );
    _rightClickMenu->addAction( editMenu->menuAction() );

    Menu* interpMenu = new Menu(_rightClickMenu);
    //interpMenu->setFont( QFont(appFont,appFontSize) );
    interpMenu->setTitle( tr("Interpolation") );
    _rightClickMenu->addAction( interpMenu->menuAction() );

    Menu* viewMenu = new Menu(_rightClickMenu);
    //viewMenu->setFont( QFont(appFont,appFontSize) );
    viewMenu->setTitle( tr("View") );
    _rightClickMenu->addAction( viewMenu->menuAction() );

    CurveEditor* ce = 0;
    if ( _widget->parentWidget() ) {
        QWidget* parent  = _widget->parentWidget()->parentWidget();
        if (parent) {
            if ( parent->objectName() == QString::fromUtf8("CurveEditor") ) {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }

    Menu* predefMenu  = 0;
    if (ce) {
        predefMenu = new Menu(_rightClickMenu);
        predefMenu->setTitle( tr("Predefined") );
        _rightClickMenu->addAction( predefMenu->menuAction() );
    }

    Menu* optionsMenu = new Menu(_rightClickMenu);
    optionsMenu->setTitle( tr("Options") );
    _rightClickMenu->addAction( optionsMenu->menuAction() );


    QAction* exportCurveToAsciiAction = new QAction(tr("Export curve to ASCII file"), fileMenu);
    QObject::connect( exportCurveToAsciiAction, SIGNAL(triggered()), _widget, SLOT(exportCurveToAscii()) );
    fileMenu->addAction(exportCurveToAsciiAction);

    QAction* importCurveFromAsciiAction = new QAction(tr("Import curve from ASCII file"), fileMenu);
    QObject::connect( importCurveFromAsciiAction, SIGNAL(triggered()), _widget, SLOT(importCurveFromAscii()) );
    fileMenu->addAction(importCurveFromAsciiAction);

    QAction* deleteKeyFramesAction = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys,
                                                            kShortcutDescActionCurveEditorRemoveKeys, editMenu);
    deleteKeyFramesAction->setShortcut( QKeySequence(Qt::Key_Backspace) );
    QObject::connect( deleteKeyFramesAction, SIGNAL(triggered()), _widget, SLOT(deleteSelectedKeyFrames()) );
    editMenu->addAction(deleteKeyFramesAction);

    QAction* copyKeyFramesAction = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCopy,
                                                          kShortcutDescActionCurveEditorCopy, editMenu);
    copyKeyFramesAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_C) );

    QObject::connect( copyKeyFramesAction, SIGNAL(triggered()), _widget, SLOT(copySelectedKeyFramesToClipBoard()) );
    editMenu->addAction(copyKeyFramesAction);

    QAction* pasteKeyFramesAction = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorPaste,
                                                           kShortcutDescActionCurveEditorPaste, editMenu);
    pasteKeyFramesAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_V) );
    QObject::connect( pasteKeyFramesAction, SIGNAL(triggered()), _widget, SLOT(pasteKeyFramesFromClipBoardToSelectedCurve()) );
    editMenu->addAction(pasteKeyFramesAction);

    QAction* selectAllAction = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll,
                                                      kShortcutDescActionCurveEditorSelectAll, editMenu);
    selectAllAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_A) );
    QObject::connect( selectAllAction, SIGNAL(triggered()), _widget, SLOT(selectAllKeyFrames()) );
    editMenu->addAction(selectAllAction);


    QAction* constantInterp = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorConstant,
                                                     kShortcutDescActionCurveEditorConstant, interpMenu);
    constantInterp->setShortcut( QKeySequence(Qt::Key_K) );
    QObject::connect( constantInterp, SIGNAL(triggered()), _widget, SLOT(constantInterpForSelectedKeyFrames()) );
    interpMenu->addAction(constantInterp);

    QAction* linearInterp = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorLinear,
                                                   kShortcutDescActionCurveEditorLinear, interpMenu);
    linearInterp->setShortcut( QKeySequence(Qt::Key_L) );
    QObject::connect( linearInterp, SIGNAL(triggered()), _widget, SLOT(linearInterpForSelectedKeyFrames()) );
    interpMenu->addAction(linearInterp);


    QAction* smoothInterp = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSmooth,
                                                   kShortcutDescActionCurveEditorSmooth, interpMenu);
    smoothInterp->setShortcut( QKeySequence(Qt::Key_Z) );
    QObject::connect( smoothInterp, SIGNAL(triggered()), _widget, SLOT(smoothForSelectedKeyFrames()) );
    interpMenu->addAction(smoothInterp);


    QAction* catmullRomInterp = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCatmullrom,
                                                       kShortcutDescActionCurveEditorCatmullrom, interpMenu);
    catmullRomInterp->setShortcut( QKeySequence(Qt::Key_R) );
    QObject::connect( catmullRomInterp, SIGNAL(triggered()), _widget, SLOT(catmullromInterpForSelectedKeyFrames()) );
    interpMenu->addAction(catmullRomInterp);


    QAction* cubicInterp = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCubic,
                                                  kShortcutDescActionCurveEditorCubic, interpMenu);
    cubicInterp->setShortcut( QKeySequence(Qt::Key_C) );
    QObject::connect( cubicInterp, SIGNAL(triggered()), _widget, SLOT(cubicInterpForSelectedKeyFrames()) );
    interpMenu->addAction(cubicInterp);

    QAction* horizontalInterp = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorHorizontal,
                                                       kShortcutDescActionCurveEditorHorizontal, interpMenu);
    horizontalInterp->setShortcut( QKeySequence(Qt::Key_H) );
    QObject::connect( horizontalInterp, SIGNAL(triggered()), _widget, SLOT(horizontalInterpForSelectedKeyFrames()) );
    interpMenu->addAction(horizontalInterp);


    QAction* breakDerivatives = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorBreak,
                                                       kShortcutDescActionCurveEditorBreak, interpMenu);
    breakDerivatives->setShortcut( QKeySequence(Qt::Key_X) );
    QObject::connect( breakDerivatives, SIGNAL(triggered()), _widget, SLOT(breakDerivativesForSelectedKeyFrames()) );
    interpMenu->addAction(breakDerivatives);

    QAction* frameAll = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenterAll,
                                               kShortcutDescActionCurveEditorCenterAll, interpMenu);
    frameAll->setShortcut( QKeySequence(Qt::Key_A) );
    QObject::connect( frameAll, SIGNAL(triggered()), _widget, SLOT(frameAll()) );
    viewMenu->addAction(frameAll);

    QAction* frameCurve = new ActionWithShortcut(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenter,
                                                 kShortcutDescActionCurveEditorCenter, interpMenu);
    frameCurve->setShortcut( QKeySequence(Qt::Key_F) );
    QObject::connect( frameCurve, SIGNAL(triggered()), _widget, SLOT(frameSelectedCurve()) );
    viewMenu->addAction(frameCurve);

    if (predefMenu) {
        QAction* loop = new QAction(tr("Loop"), _rightClickMenu);
        QObject::connect( loop, SIGNAL(triggered()), _widget, SLOT(loopSelectedCurve()) );
        predefMenu->addAction(loop);

        QAction* reverse = new QAction(tr("Reverse"), _rightClickMenu);
        QObject::connect( reverse, SIGNAL(triggered()), _widget, SLOT(reverseSelectedCurve()) );
        predefMenu->addAction(reverse);


        QAction* negate = new QAction(tr("Negate"), _rightClickMenu);
        QObject::connect( negate, SIGNAL(triggered()), _widget, SLOT(negateSelectedCurve()) );
        predefMenu->addAction(negate);
    }

    QAction* updateOnPenUp = new QAction(tr("Update on mouse release only"), _rightClickMenu);
    updateOnPenUp->setCheckable(true);
    updateOnPenUp->setChecked( appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly() );
    optionsMenu->addAction(updateOnPenUp);
    QObject::connect( updateOnPenUp, SIGNAL(triggered()), _widget, SLOT(onUpdateOnPenUpActionTriggered()) );
} // createMenu

void
CurveWidgetPrivate::drawSelectionRectangle()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::glEnable(GL_LINE_SMOOTH);
        GL_GPU::glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::glColor4f(0.3, 0.3, 0.3, 0.2);
        QPointF btmRight = _selectionRectangle.bottomRight();
        QPointF topLeft = _selectionRectangle.topLeft();

        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::glVertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::glEnd();


        GL_GPU::glLineWidth(1.5);

        GL_GPU::glColor4f(0.5, 0.5, 0.5, 1.);
        GL_GPU::glBegin(GL_LINE_LOOP);
        GL_GPU::glVertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::glVertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::glEnd();

        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
}


void
CurveWidgetPrivate::drawTimelineMarkers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );
    glCheckError(GL_GPU);

    TimeLinePtr timeline = _model.lock()->getTimeline();
    if (!timeline) {
        return;
    }

    double cursorR, cursorG, cursorB;
    double boundsR, boundsG, boundsB;
    SettingsPtr settings = appPTR->getCurrentSettings();
    settings->getTimelinePlayheadColor(&cursorR, &cursorG, &cursorB);
    settings->getTimelineBoundsColor(&boundsR, &boundsG, &boundsB);

    QPointF topLeft = zoomCtx.toZoomCoordinates(0, 0);
    QPointF btmRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, _widget->height() - 1);

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::glEnable(GL_LINE_SMOOTH);
        GL_GPU::glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        GL_GPU::glColor4f(boundsR, boundsG, boundsB, 1.);

        double leftBound, rightBound;
        _gui->getApp()->getFrameRange(&leftBound, &rightBound);
        GL_GPU::glBegin(GL_LINES);
        GL_GPU::glVertex2f( leftBound, btmRight.y() );
        GL_GPU::glVertex2f( leftBound, topLeft.y() );
        GL_GPU::glVertex2f( rightBound, btmRight.y() );
        GL_GPU::glVertex2f( rightBound, topLeft.y() );
        GL_GPU::glColor4f(cursorR, cursorG, cursorB, 1.);
        GL_GPU::glVertex2f( timeline->currentFrame(), btmRight.y() );
        GL_GPU::glVertex2f( timeline->currentFrame(), topLeft.y() );
        GL_GPU::glEnd();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        GL_GPU::glEnable(GL_POLYGON_SMOOTH);
        GL_GPU::glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

        QPointF topLeft = zoomCtx.toZoomCoordinates(0, 0);
        QPointF btmRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, _widget->height() - 1);
        QPointF btmCursorBtm( timeline->currentFrame(), btmRight.y() );
        QPointF btmcursorBtmWidgetCoord = zoomCtx.toWidgetCoordinates( btmCursorBtm.x(), btmCursorBtm.y() );
        QPointF btmCursorTop = zoomCtx.toZoomCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - CURSOR_HEIGHT);
        QPointF btmCursorLeft = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() - CURSOR_WIDTH / 2., btmcursorBtmWidgetCoord.y() );
        QPointF btmCursorRight = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() + CURSOR_WIDTH / 2., btmcursorBtmWidgetCoord.y() );
        QPointF topCursortop( timeline->currentFrame(), topLeft.y() );
        QPointF topcursorTopWidgetCoord = zoomCtx.toWidgetCoordinates( topCursortop.x(), topCursortop.y() );
        QPointF topCursorBtm = zoomCtx.toZoomCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + CURSOR_HEIGHT);
        QPointF topCursorLeft = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() - CURSOR_WIDTH / 2., topcursorTopWidgetCoord.y() );
        QPointF topCursorRight = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() + CURSOR_WIDTH / 2., topcursorTopWidgetCoord.y() );


        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f( btmCursorTop.x(), btmCursorTop.y() );
        GL_GPU::glVertex2f( btmCursorLeft.x(), btmCursorLeft.y() );
        GL_GPU::glVertex2f( btmCursorRight.x(), btmCursorRight.y() );
        GL_GPU::glEnd();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f( topCursorBtm.x(), topCursorBtm.y() );
        GL_GPU::glVertex2f( topCursorLeft.x(), topCursorLeft.y() );
        GL_GPU::glVertex2f( topCursorRight.x(), topCursorRight.y() );
        GL_GPU::glEnd();
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
    glCheckErrorIgnoreOSXBug(GL_GPU);
} // CurveWidgetPrivate::drawTimelineMarkers

void
CurveWidgetPrivate::drawCurves()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    //now draw each curve
    const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();

    std::list<CurveGuiPtr> curves;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (guiCurve) {
            curves.push_back(guiCurve);
        }
    }
    int i = 0;
    int nCurves = (int)curves.size();
    for (std::list<CurveGuiPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it, ++i) {
        (*it)->drawCurve(i, nCurves);
    }
}

void
CurveWidgetPrivate::drawScale()
{
    glCheckError(GL_GPU);
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    QPointF btmLeft = zoomCtx.toZoomCoordinates(0, _widget->height() - 1);
    QPointF topRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, 0);

    ///don't attempt to draw a scale on a widget with an invalid height/width
    if ( (_widget->height() <= 1) || (_widget->width() <= 1) ) {
        return;
    }

    QFontMetrics fontM = _widget->fontMetrics();
    const double smallestTickSizePixel = 10.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 500.; // tick size (in pixels) for alpha = 1.
    double gridR, gridG, gridB;
    SettingsPtr sett = appPTR->getCurrentSettings();
    sett->getCurveEditorGridColor(&gridR, &gridG, &gridB);


    double scaleR, scaleG, scaleB;
    sett->getCurveEditorScaleColor(&scaleR, &scaleG, &scaleB);

    QColor scaleColor;
    scaleColor.setRgbF( Image::clamp(scaleR, 0., 1.),
                        Image::clamp(scaleG, 0., 1.),
                        Image::clamp(scaleB, 0., 1.) );


    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (int axis = 0; axis < 2; ++axis) {
            const double rangePixel = (axis == 0) ? _widget->width() : _widget->height(); // AXIS-SPECIFIC
            const double range_min = (axis == 0) ? btmLeft.x() : btmLeft.y(); // AXIS-SPECIFIC
            const double range_max = (axis == 0) ? topRight.x() : topRight.y(); // AXIS-SPECIFIC
            const double range = range_max - range_min;
            double smallTickSize;
            bool half_tick;
            ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);
            int m1, m2;
            const int ticks_max = 1000;
            double offset;
            ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
            std::vector<int> ticks;
            ticks_fill(half_tick, ticks_max, m1, m2, &ticks);
            const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
            const double largestTickSize = range * largestTickSizePixel / rangePixel;
            const double minTickSizeTextPixel = (axis == 0) ? fontM.width( QLatin1String("00") ) : fontM.height(); // AXIS-SPECIFIC
            const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
            for (int i = m1; i <= m2; ++i) {
                double value = i * smallTickSize + offset;
                const double tickSize = ticks[i - m1] * smallTickSize;
                const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

                glCheckError(GL_GPU);
                GL_GPU::glColor4f(gridR, gridG, gridB, alpha);

                GL_GPU::glBegin(GL_LINES);
                if (axis == 0) {
                    GL_GPU::glVertex2f( value, btmLeft.y() ); // AXIS-SPECIFIC
                    GL_GPU::glVertex2f( value, topRight.y() ); // AXIS-SPECIFIC
                } else {
                    GL_GPU::glVertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                    GL_GPU::glVertex2f(topRight.x(), value); // AXIS-SPECIFIC
                }
                GL_GPU::glEnd();
                glCheckErrorIgnoreOSXBug(GL_GPU);

                if (tickSize > minTickSizeText) {
                    const int tickSizePixel = rangePixel * tickSize / range;
                    const QString s = QString::number(value);
                    const int sSizePixel = (axis == 0) ? fontM.width(s) : fontM.height(); // AXIS-SPECIFIC
                    if (tickSizePixel > sSizePixel) {
                        const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                        double alphaText = 1.0; //alpha;
                        if (tickSizePixel < sSizeFullPixel) {
                            // when the text size is between sSizePixel and sSizeFullPixel,
                            // draw it with a lower alpha
                            alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                        }
                        alphaText = std::min(alphaText, alpha); // don't draw more opaque than tcks
                        QColor c = scaleColor;
                        c.setAlpha(255 * alphaText);
                        if (axis == 0) {
                            _widget->renderText(value, btmLeft.y(), s, c, _widget->font(), Qt::AlignHCenter); // AXIS-SPECIFIC
                        } else {
                            _widget->renderText(btmLeft.x(), value, s, c, _widget->font(), Qt::AlignVCenter); // AXIS-SPECIFIC
                        }
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);


    glCheckError(GL_GPU);
    GL_GPU::glColor4f(gridR, gridG, gridB, 1.);
    GL_GPU::glBegin(GL_LINES);
    GL_GPU::glVertex2f(AXIS_MIN, 0);
    GL_GPU::glVertex2f(AXIS_MAX, 0);
    GL_GPU::glVertex2f(0, AXIS_MIN);
    GL_GPU::glVertex2f(0, AXIS_MAX);
    GL_GPU::glEnd();


    glCheckErrorIgnoreOSXBug(GL_GPU);
} // drawScale

void
CurveWidgetPrivate::drawSelectedKeyFramesBbox()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::glEnable(GL_LINE_SMOOTH);
        GL_GPU::glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        QPointF topLeft = _selectedKeyFramesBbox.topLeft();
        QPointF btmRight = _selectedKeyFramesBbox.bottomRight();
        QPointF topLeftWidget = zoomCtx.toWidgetCoordinates( topLeft.x(), topLeft.y() );
        QPointF btmRightWidget = zoomCtx.toWidgetCoordinates( btmRight.x(), btmRight.y() );
        double xMid = ( topLeft.x() + btmRight.x() ) / 2.;
        double yMid = ( topLeft.y() + btmRight.y() ) / 2.;

        GL_GPU::glLineWidth(1.5);

        GL_GPU::glColor4f(0.5, 0.5, 0.5, 1.);
        GL_GPU::glBegin(GL_LINE_LOOP);
        GL_GPU::glVertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::glVertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::glEnd();


        GL_GPU::glBegin(GL_LINES);
        GL_GPU::glVertex2f( std::max( _selectedKeyFramesCrossHorizLine.p1().x(), topLeft.x() ), _selectedKeyFramesCrossHorizLine.p1().y() );
        GL_GPU::glVertex2f( std::min( _selectedKeyFramesCrossHorizLine.p2().x(), btmRight.x() ), _selectedKeyFramesCrossHorizLine.p2().y() );
        GL_GPU::glVertex2f( _selectedKeyFramesCrossVertLine.p1().x(), std::max( _selectedKeyFramesCrossVertLine.p1().y(), btmRight.y() ) );
        GL_GPU::glVertex2f( _selectedKeyFramesCrossVertLine.p2().x(), std::min( _selectedKeyFramesCrossVertLine.p2().y(), topLeft.y() ) );

        //top tick
        {
            double yBottom = zoomCtx.toZoomCoordinates(0, topLeftWidget.y() + BOUNDING_BOX_HANDLE_SIZE).y();
            double yTop = zoomCtx.toZoomCoordinates(0, topLeftWidget.y() - BOUNDING_BOX_HANDLE_SIZE).y();
            GL_GPU::glVertex2f(xMid, yBottom);
            GL_GPU::glVertex2f(xMid, yTop);
        }
        //left tick
        {
            double xLeft = zoomCtx.toZoomCoordinates(topLeftWidget.x() - BOUNDING_BOX_HANDLE_SIZE, 0).x();
            double xRight = zoomCtx.toZoomCoordinates(topLeftWidget.x() + BOUNDING_BOX_HANDLE_SIZE, 0).x();
            GL_GPU::glVertex2f(xLeft, yMid);
            GL_GPU::glVertex2f(xRight, yMid);
        }
        //bottom tick
        {
            double yBottom = zoomCtx.toZoomCoordinates(0, btmRightWidget.y() + BOUNDING_BOX_HANDLE_SIZE).y();
            double yTop = zoomCtx.toZoomCoordinates(0, btmRightWidget.y() - BOUNDING_BOX_HANDLE_SIZE).y();
            GL_GPU::glVertex2f(xMid, yBottom);
            GL_GPU::glVertex2f(xMid, yTop);
        }
        //right tick
        {
            double xLeft = zoomCtx.toZoomCoordinates(btmRightWidget.x() - BOUNDING_BOX_HANDLE_SIZE, 0).x();
            double xRight = zoomCtx.toZoomCoordinates(btmRightWidget.x() + BOUNDING_BOX_HANDLE_SIZE, 0).x();
            GL_GPU::glVertex2f(xLeft, yMid);
            GL_GPU::glVertex2f(xRight, yMid);
        }
        GL_GPU::glEnd();

        GL_GPU::glPointSize(BOUNDING_BOX_HANDLE_SIZE);
        GL_GPU::glBegin(GL_POINTS);
        GL_GPU::glVertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::glVertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::glVertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::glEnd();

        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
} // CurveWidgetPrivate::drawSelectedKeyFramesBbox

CurveGuiPtr
CurveWidgetPrivate::isNearbyCurve(const QPoint &pt,
                                  double* x,
                                  double *y) const
{
    QPointF curvePos = zoomCtx.toZoomCoordinates( pt.x(), pt.y() );

    
   const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
    
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }

        
        double yCurve = 0;
        
        try {
            yCurve = it->first.item->evaluateCurve( true /*useExpression*/, curvePos.x(), guiCurve->getDimension(), guiCurve->getView());
        } catch (...) {
        }
        
        double yWidget = zoomCtx.toWidgetCoordinates(0, yCurve).y();
        if ( (pt.y() < yWidget + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && (pt.y() > yWidget - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) {
            if (x != NULL) {
                *x = curvePos.x();
            }
            if (y != NULL) {
                *y = yCurve;
            }
            
            return guiCurve;
        }
    }

    return CurveGuiPtr();
} // isNearbyCurve

AnimItemDimViewAndTime
CurveWidgetPrivate::isNearbyKeyFrame(const QPoint & pt) const
{
    AnimItemDimViewAndTime ret;
    
    const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }
        KeyFrameSet set = guiCurve->getKeyFrames();

        for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            QPointF keyFramewidgetPos = zoomCtx.toWidgetCoordinates( it2->getTime(), it2->getValue() );
            if ( (std::abs( pt.y() - keyFramewidgetPos.y() ) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                (std::abs( pt.x() - keyFramewidgetPos.x() ) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) {
                
                ret.time = it2->getTime();
                ret.id = it->first;
                
                return ret;
            }
           
        }

    } // for all curves
    return ret;
} // CurveWidgetPrivate::isNearbyKeyFrame

AnimItemDimViewAndTime
CurveWidgetPrivate::isNearbyKeyFrameText(const QPoint& pt) const
{
    AnimItemDimViewAndTime ret;
    
    QFontMetrics fm( _widget->font() );
    int yOffset = 4;
    const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();

    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }
        KeyFrameSet set = guiCurve->getKeyFrames();
        // Bail if multiple selection because text is not visible
        if (set.size() > 1) {
            return ret;
        }

        for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            
            QPointF topLeftWidget = zoomCtx.toWidgetCoordinates( it2->getTime(), it2->getValue() );
            topLeftWidget.ry() += yOffset;
            
            QString coordStr =  QString::fromUtf8("x: %1, y: %2").arg( it2->getTime() ).arg( it2->getValue() );
            QPointF btmRightWidget( topLeftWidget.x() + fm.width(coordStr), topLeftWidget.y() + fm.height() );
            
            if ( (pt.x() >= topLeftWidget.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && (pt.x() <= btmRightWidget.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                ( pt.y() >= topLeftWidget.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && ( pt.y() <= btmRightWidget.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) {
                ret.time = it2->getTime();
                ret.id = it->first;
                
                return ret;
            }

            
        }
        
    } // for all curves
    return ret;
}

std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewAndTime >
CurveWidgetPrivate::isNearbyTangent(const QPoint & pt) const
{
    
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewAndTime > ret;

    const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }
        KeyFrameSet set = guiCurve->getKeyFrames();
        
        for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            
            QPointF leftTanPos, rightTanPos;
            _widget->getKeyTangentPoints(it2, set, &leftTanPos, &rightTanPos);
            QPointF leftTanPt = zoomCtx.toWidgetCoordinates(leftTanPos.x(), leftTanPos.y());
            QPointF rightTanPt = zoomCtx.toWidgetCoordinates( rightTanPos.x(), rightTanPos.y());
            if ( ( pt.x() >= (leftTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                ( pt.x() <= (leftTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                ( pt.y() <= (leftTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                ( pt.y() >= (leftTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
                
                ret.second.time = it2->getTime();
                ret.second.id = it->first;
                ret.first = MoveTangentCommand::eSelectedTangentLeft;
                return ret;
            } else if ( ( pt.x() >= (rightTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                       ( pt.x() <= (rightTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                       ( pt.y() <= (rightTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                       ( pt.y() >= (rightTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
                ret.second.time = it2->getTime();
                ret.second.id = it->first;
                ret.first = MoveTangentCommand::eSelectedTangentRight;
                return ret;
            }
        }
        
    } // for all curves
    return ret;
    
}

std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewAndTime >
CurveWidgetPrivate::isNearbySelectedTangentText(const QPoint & pt) const
{
    QFontMetrics fm( _widget->font() );
    int yOffset = 4;
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewAndTime > ret;
    
    const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
    
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }
        KeyFrameSet set = guiCurve->getKeyFrames();
        // Bail if multiple selection because text is not visible
        if (set.size() > 1) {
            return ret;
        }
        for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            
            QPointF leftTanPos, rightTanPos;
            _widget->getKeyTangentPoints(it2, set, &leftTanPos, &rightTanPos);
            
            double rounding = std::pow(10., CURVEWIDGET_DERIVATIVE_ROUND_PRECISION);
            QPointF topLeft_LeftTanWidget = zoomCtx.toWidgetCoordinates( leftTanPos.x(), leftTanPos.y() );
            QPointF topLeft_RightTanWidget = zoomCtx.toWidgetCoordinates( rightTanPos.x(), rightTanPos.y() );
            topLeft_LeftTanWidget.ry() += yOffset;
            topLeft_RightTanWidget.ry() += yOffset;
            
            QString leftCoordStr =  QString( tr("l: %1") ).arg(std::floor( ( it2->getLeftDerivative() * rounding ) + 0.5 ) / rounding);
            QString rightCoordStr =  QString( tr("r: %1") ).arg(std::floor( ( it2->getRightDerivative() * rounding ) + 0.5 ) / rounding);
            QPointF btmRight_LeftTanWidget( topLeft_LeftTanWidget.x() + fm.width(leftCoordStr), topLeft_LeftTanWidget.y() + fm.height() );
            QPointF btmRight_RightTanWidget( topLeft_RightTanWidget.x() + fm.width(rightCoordStr), topLeft_RightTanWidget.y() + fm.height() );
            
            if ( (pt.x() >= topLeft_LeftTanWidget.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && (pt.x() <= btmRight_LeftTanWidget.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                ( pt.y() >= topLeft_LeftTanWidget.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && ( pt.y() <= btmRight_LeftTanWidget.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) {
                ret.second.time = it2->getTime();
                ret.second.id = it->first;
                ret.first = MoveTangentCommand::eSelectedTangentLeft;
            } else if ( (pt.x() >= topLeft_RightTanWidget.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && (pt.x() <= btmRight_RightTanWidget.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                       ( pt.y() >= topLeft_RightTanWidget.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) && ( pt.y() <= btmRight_RightTanWidget.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) {
                ret.second.time = it2->getTime();
                ret.second.id = it->first;
                ret.first = MoveTangentCommand::eSelectedTangentRight;
            }
            
        }
        
    } // for all curves
    return ret;

} // isNearbySelectedTangentText

bool
CurveWidgetPrivate::isNearbySelectedKeyFramesCrossWidget(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF middleLeft = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossHorizLine.p1().x(), _selectedKeyFramesCrossHorizLine.p1().y() );
    QPointF middleRight = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossHorizLine.p2().x(), _selectedKeyFramesCrossHorizLine.p2().y() );
    QPointF middleBtm = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossVertLine.p1().x(), _selectedKeyFramesCrossVertLine.p1().y() );
    QPointF middleTop = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossVertLine.p2().x(), _selectedKeyFramesCrossVertLine.p2().y() );

    if ( ( pt.x() >= (middleLeft.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (middleRight.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (middleLeft.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (middleLeft.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        //is nearby horizontal line
        return true;
    } else if ( ( pt.y() >= (middleBtm.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                ( pt.y() <= (middleTop.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                ( pt.x() <= (middleBtm.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                ( pt.x() >= (middleBtm.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        //is nearby vertical line
        return true;
    } else {
        return false;
    }
}

bool
CurveWidgetPrivate::isNearbyBboxTopLeft(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates( _selectedKeyFramesBbox.x(), _selectedKeyFramesBbox.y() );

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxMidLeft(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x(),
                                                _selectedKeyFramesBbox.y() + _selectedKeyFramesBbox.height() / 2.);

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxBtmLeft(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates( _selectedKeyFramesBbox.x(), _selectedKeyFramesBbox.y() +
                                                 _selectedKeyFramesBbox.height() );

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxMidBtm(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates( _selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width() / 2., _selectedKeyFramesBbox.y() +
                                                 _selectedKeyFramesBbox.height() );

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxBtmRight(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates( _selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width(), _selectedKeyFramesBbox.y() +
                                                 _selectedKeyFramesBbox.height() );

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxMidRight(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width(), _selectedKeyFramesBbox.y() +
                                                _selectedKeyFramesBbox.height() / 2.);

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxTopRight(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates( _selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width(), _selectedKeyFramesBbox.y() );

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyBboxMidTop(const QPoint& pt) const
{
    QPointF other = zoomCtx.toWidgetCoordinates( _selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width() / 2., _selectedKeyFramesBbox.y() );

    if ( ( pt.x() >= (other.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.x() <= (other.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() <= (other.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
         ( pt.y() >= (other.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
        return true;
    }

    return false;
}

bool
CurveWidgetPrivate::isNearbyTimelineTopPoly(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    TimeLinePtr timeline = _model.lock()->getTimeline();
    if (!timeline) {
        return false;
    }
    
    QPointF pt_opengl = zoomCtx.toZoomCoordinates( pt.x(), pt.y() );
    
    QPointF topLeft = zoomCtx.toZoomCoordinates(0, 0);
    QPointF topCursortop( timeline->currentFrame(), topLeft.y() );
    QPointF topcursorTopWidgetCoord = zoomCtx.toWidgetCoordinates( topCursortop.x(), topCursortop.y() );
    QPointF topCursorBtm = zoomCtx.toZoomCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + CURSOR_HEIGHT);
    QPointF topCursorLeft = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() - CURSOR_WIDTH / 2., topcursorTopWidgetCoord.y() );
    QPointF topCursorRight = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() + CURSOR_WIDTH / 2., topcursorTopWidgetCoord.y() );
    
    QPolygonF poly;
    poly.push_back(topCursorBtm);
    poly.push_back(topCursorLeft);
    poly.push_back(topCursorRight);

    return poly.containsPoint(pt_opengl, Qt::OddEvenFill);
}

bool
CurveWidgetPrivate::isNearbyTimelineBtmPoly(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    TimeLinePtr timeline = _model.lock()->getTimeline();
    if (!timeline) {
        return false;
    }
    QPointF pt_opengl = zoomCtx.toZoomCoordinates( pt.x(), pt.y() );

    QPointF btmRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, _widget->height() - 1);
    QPointF btmCursorBtm( timeline->currentFrame(), btmRight.y() );
    QPointF btmcursorBtmWidgetCoord = zoomCtx.toWidgetCoordinates( btmCursorBtm.x(), btmCursorBtm.y() );
    QPointF btmCursorTop = zoomCtx.toZoomCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - CURSOR_HEIGHT);
    QPointF btmCursorLeft = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() - CURSOR_WIDTH / 2., btmcursorBtmWidgetCoord.y() );
    QPointF btmCursorRight = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() + CURSOR_WIDTH / 2., btmcursorBtmWidgetCoord.y() );
    
    QPolygonF poly;
    poly.push_back(btmCursorTop);
    poly.push_back(btmCursorLeft);
    poly.push_back(btmCursorRight);
    
    return poly.containsPoint(pt_opengl, Qt::OddEvenFill);
}

void
CurveWidgetPrivate::keyFramesWithinRect(const QRectF & rect,
                                        AnimItemDimViewKeyFramesMap* keys) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double left = rect.topLeft().x();
    double right = rect.bottomRight().x();
    double bottom = rect.topLeft().y();
    double top = rect.bottomRight().y();
    
    const AnimItemDimViewKeyFramesMap& selectedKeys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
    
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }
        
        KeyFrameSet set = guiCurve->getKeyFrames();
        if ( set.empty() ) {
            continue;
        }
        
        StringAnimationManagerPtr stringAnim = it->first.item->getInternalAnimItem()->getStringAnimation();
        
        
        KeyFrameWithStringSet& outKeys = (*keys)[it->first];
        
        for ( KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            double y = it2->getValue();
            double x = it2->getTime();
            if ( (x <= right) && (x >= left) && (y <= top) && (y >= bottom) ) {
                //KeyPtr newSelectedKey( new SelectedKey(*it, *it2, hasPrev, prevKey, hasNext, nextKey) );
                KeyFrameWithString k;
                k.key = *it2;
                if (stringAnim) {
                    stringAnim->stringFromInterpolatedIndex(it2->getValue(), it->first.view, &k.string);
                }
                outKeys.insert(k);
            }
        }
        
    }
} // CurveWidgetPrivate::keyFramesWithinRect

namespace  {
struct SortIncreasingFunctor
{
    bool operator() (const MoveKeysCommand::KeyToMove& lhs,
                     const MoveKeysCommand::KeyToMove& rhs)
    {
        return lhs.key->key.getTime() < rhs.key->key.getTime();
    }
};

struct SortDecreasingFunctor
{
    bool operator() (const MoveKeysCommand::KeyToMove& lhs,
                     const MoveKeysCommand::KeyToMove& rhs)
    {
        return lhs.key->key.getTime() > rhs.key->key.getTime();
    }
};
}


void
CurveWidgetPrivate::moveSelectedKeyFrames(const QPointF & oldClick_opengl,
                                          const QPointF & newClick_opengl)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF dragStartPointOpenGL = zoomCtx.toZoomCoordinates( _dragStartPoint.x(), _dragStartPoint.y() );
    bool clampToIntegers = _selectedKeyFrames.begin()->first->areKeyFramesTimeClampedToIntegers();
    bool useOneDirectionOnly = qApp->keyboardModifiers().testFlag(Qt::ControlModifier) || clampToIntegers;
    QPointF totalMovement;
    if (!useOneDirectionOnly) {
        totalMovement.rx() = newClick_opengl.x() - oldClick_opengl.x();
        totalMovement.ry() = newClick_opengl.y() - oldClick_opengl.y();
    } else {
        totalMovement.rx() = newClick_opengl.x() - dragStartPointOpenGL.x();
        totalMovement.ry() = newClick_opengl.y() - dragStartPointOpenGL.y();
    }


    /// round to the nearest integer the keyframes total motion (in X only)
    ///Only for the curve editor, parametric curves are not affected by the following
    if (clampToIntegers) {
        totalMovement.rx() = std::floor(totalMovement.x() + 0.5);
        for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
            if ( it->first->areKeyFramesValuesClampedToBooleans() ) {
                totalMovement.ry() = std::max( 0., std::min(std::floor(totalMovement.y() + 0.5), 1.) );
                break;
            } else if ( it->first->areKeyFramesValuesClampedToIntegers() ) {
                totalMovement.ry() = std::floor(totalMovement.y() + 0.5);
            }
        }
    }

    double dt;


    ///We also want them to allow the user to move freely the keyframes around, hence the !clampToIntegers
    if ( (_mouseDragOrientation.x() != 0) || !useOneDirectionOnly ) {
        if (!useOneDirectionOnly) {
            dt =  totalMovement.x();
        } else {
            dt = totalMovement.x() - _keyDragLastMovement.x();
        }
    } else {
        dt = 0;
    }

    // clamp dt so keyframes do not overlap
    double maxLeft = INT_MIN;
    double maxRight = INT_MAX;
    double epsilon = clampToIntegers ? 1 : 1e-4;
    bool canMoveY = true;
    std::map<CurveGuiPtr, std::vector<MoveKeysCommand::KeyToMove> > keysToMove;

    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        if ( canMoveY && !it->first->isYComponentMovable() ) {
            canMoveY = false;
        }

        KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( it->first.get() );
        if (isKnobCurve) {
            KnobGuiPtr knobUI = isKnobCurve->getKnobGui();
            if (knobUI) {
                int curveDim = isKnobCurve->getDimension();
                KnobIPtr internalKnob = knobUI->getKnob();
                if ( internalKnob && ( !internalKnob->isEnabled(curveDim) || internalKnob->isSlave(curveDim) ) ) {
                    continue;
                }
            }
        }

        CurvePtr curve = it->first->getInternalCurve();
        std::vector<MoveKeysCommand::KeyToMove>& vect = keysToMove[it->first];
        if (curve) {
            for (std::list<KeyPtr>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                MoveKeysCommand::KeyToMove keyToMove;
                keyToMove.key = *it2;
                keyToMove.prevIsSelected = false;
                keyToMove.nextIsSelected = false;
                if ( (*it2)->hasNext ) {
                    if ( !_widget->isSelectedKey( it->first, (*it2)->nextKey.getTime() ) ) {
                        double diff = (*it2)->nextKey.getTime() - (*it2)->key.getTime() - epsilon;
                        maxRight = std::max( 0., std::min(diff, maxRight) );
                    } else {
                        keyToMove.nextIsSelected = true;
                    }
                }
                if ( (*it2)->hasPrevious ) {
                    if ( !_widget->isSelectedKey( it->first, (*it2)->prevKey.getTime() ) ) {
                        double diff = (*it2)->prevKey.getTime()  - (*it2)->key.getTime() + epsilon;
                        maxLeft = std::min( 0., std::max(diff, maxLeft) );
                    } else {
                        keyToMove.prevIsSelected = true;
                    }
                }
                vect.push_back(keyToMove);
            }
        } else {
            BezierCPCurveGui* bezierCurve = dynamic_cast<BezierCPCurveGui*>( it->first.get() );
            assert(bezierCurve);
            if (!bezierCurve) {
                throw std::logic_error("CurveWidgetPrivate::moveSelectedKeyFrames");
            }
            std::set<double> keyframes;
            bezierCurve->getBezier()->getKeyframeTimes(&keyframes);

            for (std::list<KeyPtr>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                MoveKeysCommand::KeyToMove keyToMove;
                keyToMove.key = *it2;
                keyToMove.prevIsSelected = false;
                keyToMove.nextIsSelected = false;

                std::set<double>::iterator found = keyframes.find( (*it2)->key.getTime() );
                assert( found != keyframes.end() );
                if ( found != keyframes.begin() ) {
                    std::set<double>::iterator prev = found;
                    --prev;
                    if ( !_widget->isSelectedKey(it->first, *prev) ) {
                        double diff = *prev  - *found + epsilon;
                        maxLeft = std::min( 0., std::max(diff, maxLeft) );
                    } else {
                        keyToMove.prevIsSelected = true;
                    }
                }
                if ( found != keyframes.end() ) {
                    std::set<double>::iterator next = found;
                    ++next;
                    if ( next != keyframes.end() ) {
                        if ( !_widget->isSelectedKey(it->first, *next) ) {
                            double diff = *next - *found - epsilon;
                            maxRight = std::max( 0., std::min(diff, maxRight) );
                        } else {
                            keyToMove.nextIsSelected = true;
                        }
                    }
                }
                vect.push_back(keyToMove);
            }
        }
        //Keyframes must be sorted in order according to the user movement otherwise if keyframes are next to each other we might override
        //another keyframe.
        //Can only call sort on random iterators
        if (dt < 0) {
            std::sort( vect.begin(), vect.end(), SortIncreasingFunctor() );
        } else {
            std::sort( vect.begin(), vect.end(), SortDecreasingFunctor() );
        }
    }

    dt = std::min(dt, maxRight);
    dt = std::max(dt, maxLeft);


    double dv;
    ///Parametric curve editor (the ones of the KnobParametric) never clamp keyframes to integer in the X direction
    ///We also want them to allow the user to move freely the keyframes around, hence the !clampToIntegers
    if ( (_mouseDragOrientation.y() != 0) || !useOneDirectionOnly ) {
        if (!useOneDirectionOnly) {
            dv = totalMovement.y();
        } else {
            dv = totalMovement.y() - _keyDragLastMovement.y();
        }
        if (!canMoveY) {
            dv = 0;
        }
    } else {
        dv = 0;
    }


    if ( (dt != 0) || (dv != 0) ) {
        if ( (dt != 0) || (dv != 0) ) {
            bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
            _widget->pushUndoCommand( new MoveKeysCommand(_widget, keysToMove, dt, dv, !updateOnPenUpOnly) );
            _evaluateOnPenUp = true;
        }
    }
    //update last drag movement
    if ( (_mouseDragOrientation.x() != 0) || !useOneDirectionOnly ) {
        _keyDragLastMovement.rx() = totalMovement.x();
    }
    if ( (_mouseDragOrientation.y() != 0) || !useOneDirectionOnly ) {
        _keyDragLastMovement.ry() = totalMovement.y();
    }
} // moveSelectedKeyFrames

void
CurveWidgetPrivate::transformSelectedKeyFrames(const QPointF & oldClick_opengl,
                                               const QPointF & newClick_opengl,
                                               bool shiftHeld)
{
    if (newClick_opengl == oldClick_opengl) {
        return;
    }

    QPointF dragStartPointOpenGL = zoomCtx.toZoomCoordinates( _dragStartPoint.x(), _dragStartPoint.y() );
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
    QPointF totalMovement( newClick_opengl.x() - dragStartPointOpenGL.x(), newClick_opengl.y() - dragStartPointOpenGL.y() );
    SelectedKeys keysToTransform;


    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( it->first.get() );
        if (isKnobCurve) {
            KnobGuiPtr knobUI = isKnobCurve->getKnobGui();
            if (knobUI) {
                int curveDim = isKnobCurve->getDimension();
                KnobIPtr internalKnob = knobUI->getKnob();
                if ( internalKnob && ( !internalKnob->isEnabled(curveDim) || internalKnob->isSlave(curveDim) ) ) {
                    continue;
                }
            }
        }

        keysToTransform.insert(*it);
        if ( it->first->areKeyFramesValuesClampedToBooleans() ) {
            totalMovement.ry() = std::max( 0., std::min(std::floor(totalMovement.y() + 0.5), 1.) );
        } else if ( it->first->areKeyFramesValuesClampedToIntegers() ) {
            totalMovement.ry() = std::floor(totalMovement.y() + 0.5);
        }
    }


    double dt;
    dt = totalMovement.x() - _keyDragLastMovement.x();

    double dv;
    dv = totalMovement.y() - _keyDragLastMovement.y();


    if ( (dt != 0) || (dv != 0) ) {
        QPointF center = _selectedKeyFramesBbox.center();

        if ( shiftHeld &&
             ( ( _state == eEventStateDraggingMidRightBbox) ||
               ( _state == eEventStateDraggingMidBtmBbox) ||
               ( _state == eEventStateDraggingMidLeftBbox) ||
               ( _state == eEventStateDraggingMidTopBbox) ) ) {
            if (_state == eEventStateDraggingMidTopBbox) {
                center.ry() = _selectedKeyFramesBbox.bottom();
            } else if (_state == eEventStateDraggingMidBtmBbox) {
                center.ry() = _selectedKeyFramesBbox.top();
            } else if (_state == eEventStateDraggingMidLeftBbox) {
                center.rx() = _selectedKeyFramesBbox.right();
            } else if (_state == eEventStateDraggingMidRightBbox) {
                center.rx() = _selectedKeyFramesBbox.left();
            }
        }

        double sx = 1., sy = 1.;
        double tx = 0., ty = 0.;
        double oldX = newClick_opengl.x() - dt;
        double oldY = newClick_opengl.y() - dv;
        // the scale ratio is the ratio of distances to the center
        double prevDist = ( oldX - center.x() ) * ( oldX - center.x() ) + ( oldY - center.y() ) * ( oldY - center.y() );

        if (prevDist != 0) {
            double dist = ( newClick_opengl.x() - center.x() ) * ( newClick_opengl.x() - center.x() ) + ( newClick_opengl.y() - center.y() ) * ( newClick_opengl.y() - center.y() );
            double ratio = std::sqrt(dist / prevDist);

            if ( (_state == eEventStateDraggingBtmLeftBbox) ||
                 ( _state == eEventStateDraggingBtmRightBbox) ||
                 ( _state == eEventStateDraggingTopRightBbox) ||
                 ( _state == eEventStateDraggingTopLeftBbox) ) {
                sx *= ratio;
                sy *= ratio;
            } else {
                bool processX = _state == eEventStateDraggingMidLeftBbox || _state == eEventStateDraggingMidRightBbox;
                if (processX) {
                    sx *= ratio;
                } else {
                    sy *= ratio;
                }
            }
        }
        _widget->pushUndoCommand( new TransformKeysCommand(_widget, keysToTransform, center.x(), center.y(), tx, ty, sx, sy, !updateOnPenUpOnly) );
        _evaluateOnPenUp = true;
    }
    //update last drag movement
    if (dt != 0) {
        _keyDragLastMovement.rx() = totalMovement.x();
    }

    if (dv != 0) {
        _keyDragLastMovement.ry() = totalMovement.y();
    }
} // CurveWidgetPrivate::transformSelectedKeyFrames

void
CurveWidgetPrivate::moveSelectedTangent(const QPointF & pos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );


    KeyPtr & key = _selectedDerivative.second;
    double dy = key->key.getValue() - pos.y();
    double dx = key->key.getTime() - pos.x();


    ///If the knob is evaluating on change and _updateOnPenUpOnly is true, set it to false prior
    ///to modifying the keyframe, and set it back to its original value afterwards.
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    _widget->pushUndoCommand( new MoveTangentCommand(_widget, _selectedDerivative.first, key, dx, dy, !updateOnPenUpOnly) );
} // moveSelectedTangent



void
CurveWidgetPrivate::refreshSelectionRectangle(double x,
                                              double y)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double xmin = std::min(_dragStartPoint.x(), x);
    double xmax = std::max(_dragStartPoint.x(), x);
    double ymin = std::min(_dragStartPoint.y(), y);
    double ymax = std::max(_dragStartPoint.y(), y);
    _selectionRectangle.setBottomRight( zoomCtx.toZoomCoordinates(xmax, ymin) );
    _selectionRectangle.setTopLeft( zoomCtx.toZoomCoordinates(xmin, ymax) );

    _selectedKeyFrames.clear();
    keyFramesWithinRect(_selectionRectangle, &_selectedKeyFrames);
    _widget->refreshSelectedKeysBbox();
    _widget->refreshDisplayedTangents();
}

void
CurveWidgetPrivate::setSelectedKeysInterpolation(KeyframeTypeEnum type)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::list<KeyInterpolationChange> changes;
    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        for (std::list<KeyPtr>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KeyInterpolationChange change( (*it2)->key.getInterpolation(), type, (*it2) );
            changes.push_back(change);
        }
    }
    _widget->pushUndoCommand( new SetKeysInterpolationCommand(_widget, changes) );
}

void
CurveWidgetPrivate::insertSelectedKeyFrameConditionnaly(const KeyPtr &key)
{
    //insert it into the _selectedKeyFrames
    bool alreadySelected = false;

    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        for (std::list<KeyPtr>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ( (it->first == key->curve) && ( (*it2)->key.getTime() == key->key.getTime() ) ) {
                ///key is already selected
                alreadySelected = true;
                break;
            }
        }
    }

    if (!alreadySelected) {
        _selectedKeyFrames[key->curve].push_back(key);
    }
}

void
CurveWidgetPrivate::updateDopeSheetViewFrameRange()
{
    _gui->getAnimationModuleEditor()->centerOn( zoomCtx.left(), zoomCtx.right() );
}

NATRON_NAMESPACE_EXIT;


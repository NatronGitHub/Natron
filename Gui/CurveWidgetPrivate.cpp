/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QtCore/QThread>
#include <QApplication>
#include <QDebug>

#include "Engine/Bezier.h"
#include "Engine/TimeLine.h"
#include "Engine/Settings.h"
#include "Engine/Image.h" // Natron::clamp

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Menu.h"
#include "Gui/ticks.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

using namespace Natron;

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
// (in widget pixels)
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8


#define BOUNDING_BOX_HANDLE_SIZE 4

#define AXIS_MAX 100000.
#define AXIS_MIN -100000.



CurveWidgetPrivate::CurveWidgetPrivate(Gui* gui,
                                       CurveSelection* selectionModel,
                                       boost::shared_ptr<TimeLine> timeline,
                                       CurveWidget* widget)
    : _lastMousePos()
    , zoomCtx()
    , _state(eEventStateNone)
    , _rightClickMenu( new Menu(widget) )
    , _selectedCurveColor(255,255,89,255)
    , _nextCurveAddedColor()
    , textRenderer()
    , _font( new QFont(appFont,appFontSize) )
    , _curves()
    , _selectedKeyFrames()
    , _hasOpenGLVAOSupport(true)
    , _mustSetDragOrientation(false)
    , _mouseDragOrientation()
    , _keyFramesClipBoard()
    , _selectionRectangle()
    , _dragStartPoint()
    , _drawSelectedKeyFramesBbox(false)
    , _selectedKeyFramesBbox()
    , _selectedKeyFramesCrossVertLine()
    , _selectedKeyFramesCrossHorizLine()
    , _timeline(timeline)
    , _timelineEnabled(false)
    , _selectedDerivative()
    , _evaluateOnPenUp(false)
    , _keyDragLastMovement()
    , _undoStack(new QUndoStack)
    , _gui(gui)
    , savedTexture(0)
    , sizeH()
    , zoomOrPannedSinceLastFit(false)
    , _timelineTopPoly()
    , _timelineBtmPoly()
    , _widget(widget)
    , _selectionModel(selectionModel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _nextCurveAddedColor.setHsv(200,255,255);
    //_rightClickMenu->setFont( QFont(appFont,appFontSize) );
    _gui->registerNewUndoStack( _undoStack.get() );
    
}

CurveWidgetPrivate::~CurveWidgetPrivate()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    delete _font;
    _curves.clear();
}

void
CurveWidgetPrivate::createMenu()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _rightClickMenu->clear();

    Menu* fileMenu = new Menu(_rightClickMenu);
    //fileMenu->setFont( QFont(appFont,appFontSize) );
    fileMenu->setTitle( QObject::tr("File") );
    _rightClickMenu->addAction( fileMenu->menuAction() );

    Menu* editMenu = new Menu(_rightClickMenu);
    //editMenu->setFont( QFont(appFont,appFontSize) );
    editMenu->setTitle( QObject::tr("Edit") );
    _rightClickMenu->addAction( editMenu->menuAction() );

    Menu* interpMenu = new Menu(_rightClickMenu);
    //interpMenu->setFont( QFont(appFont,appFontSize) );
    interpMenu->setTitle( QObject::tr("Interpolation") );
    _rightClickMenu->addAction( interpMenu->menuAction() );

    Menu* viewMenu = new Menu(_rightClickMenu);
    //viewMenu->setFont( QFont(appFont,appFontSize) );
    viewMenu->setTitle( QObject::tr("View") );
    _rightClickMenu->addAction( viewMenu->menuAction() );
    
    CurveEditor* ce = 0;
    if ( _widget->parentWidget() ) {
        QWidget* parent  = _widget->parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == "CurveEditor") {
                ce = dynamic_cast<CurveEditor*>(parent);
            }
        }
    }
    
    Menu* predefMenu  = 0;
    if (ce) {
        predefMenu = new Menu(_rightClickMenu);
        predefMenu->setTitle(QObject::tr("Predefined"));
        _rightClickMenu->addAction(predefMenu->menuAction());
    }
    
    
    QAction* exportCurveToAsciiAction = new QAction(QObject::tr("Export curve to Ascii"),fileMenu);
    QObject::connect( exportCurveToAsciiAction,SIGNAL( triggered() ),_widget,SLOT( exportCurveToAscii() ) );
    fileMenu->addAction(exportCurveToAsciiAction);

    QAction* importCurveFromAsciiAction = new QAction(QObject::tr("Import curve from Ascii"),fileMenu);
    QObject::connect( importCurveFromAsciiAction,SIGNAL( triggered() ),_widget,SLOT( importCurveFromAscii() ) );
    fileMenu->addAction(importCurveFromAsciiAction);

    QAction* deleteKeyFramesAction = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorRemoveKeys,
                                                            kShortcutDescActionCurveEditorRemoveKeys,editMenu);
    deleteKeyFramesAction->setShortcut( QKeySequence(Qt::Key_Backspace) );
    QObject::connect( deleteKeyFramesAction,SIGNAL( triggered() ),_widget,SLOT( deleteSelectedKeyFrames() ) );
    editMenu->addAction(deleteKeyFramesAction);

    QAction* copyKeyFramesAction = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorCopy,
                                                          kShortcutDescActionCurveEditorCopy,editMenu);
    copyKeyFramesAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_C) );
    QObject::connect( copyKeyFramesAction,SIGNAL( triggered() ),_widget,SLOT( copySelectedKeyFrames() ) );
    editMenu->addAction(copyKeyFramesAction);

    QAction* pasteKeyFramesAction = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorPaste,
                                                           kShortcutDescActionCurveEditorPaste,editMenu);
    pasteKeyFramesAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_V) );
    QObject::connect( pasteKeyFramesAction,SIGNAL( triggered() ),_widget,SLOT( pasteKeyFramesFromClipBoardToSelectedCurve() ) );
    editMenu->addAction(pasteKeyFramesAction);

    QAction* selectAllAction = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorSelectAll,
                                                      kShortcutDescActionCurveEditorSelectAll,editMenu);
    selectAllAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_A) );
    QObject::connect( selectAllAction,SIGNAL( triggered() ),_widget,SLOT( selectAllKeyFrames() ) );
    editMenu->addAction(selectAllAction);


    QAction* constantInterp = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorConstant,
                                                     kShortcutDescActionCurveEditorConstant,interpMenu);
    constantInterp->setShortcut( QKeySequence(Qt::Key_K) );
    QObject::connect( constantInterp,SIGNAL( triggered() ),_widget,SLOT( constantInterpForSelectedKeyFrames() ) );
    interpMenu->addAction(constantInterp);

    QAction* linearInterp = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorLinear,
                                                   kShortcutDescActionCurveEditorLinear,interpMenu);
    linearInterp->setShortcut( QKeySequence(Qt::Key_L) );
    QObject::connect( linearInterp,SIGNAL( triggered() ),_widget,SLOT( linearInterpForSelectedKeyFrames() ) );
    interpMenu->addAction(linearInterp);


    QAction* smoothInterp = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorSmooth,
                                                   kShortcutDescActionCurveEditorSmooth,interpMenu);
    smoothInterp->setShortcut( QKeySequence(Qt::Key_Z) );
    QObject::connect( smoothInterp,SIGNAL( triggered() ),_widget,SLOT( smoothForSelectedKeyFrames() ) );
    interpMenu->addAction(smoothInterp);


    QAction* catmullRomInterp = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorCatmullrom,
                                                       kShortcutDescActionCurveEditorCatmullrom,interpMenu);
    catmullRomInterp->setShortcut( QKeySequence(Qt::Key_R) );
    QObject::connect( catmullRomInterp,SIGNAL( triggered() ),_widget,SLOT( catmullromInterpForSelectedKeyFrames() ) );
    interpMenu->addAction(catmullRomInterp);


    QAction* cubicInterp = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorCubic,
                                                  kShortcutDescActionCurveEditorCubic,interpMenu);
    cubicInterp->setShortcut( QKeySequence(Qt::Key_C) );
    QObject::connect( cubicInterp,SIGNAL( triggered() ),_widget,SLOT( cubicInterpForSelectedKeyFrames() ) );
    interpMenu->addAction(cubicInterp);

    QAction* horizontalInterp = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorHorizontal,
                                                       kShortcutDescActionCurveEditorHorizontal,interpMenu);
    horizontalInterp->setShortcut( QKeySequence(Qt::Key_H) );
    QObject::connect( horizontalInterp,SIGNAL( triggered() ),_widget,SLOT( horizontalInterpForSelectedKeyFrames() ) );
    interpMenu->addAction(horizontalInterp);


    QAction* breakDerivatives = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorBreak,
                                                       kShortcutDescActionCurveEditorBreak,interpMenu);
    breakDerivatives->setShortcut( QKeySequence(Qt::Key_X) );
    QObject::connect( breakDerivatives,SIGNAL( triggered() ),_widget,SLOT( breakDerivativesForSelectedKeyFrames() ) );
    interpMenu->addAction(breakDerivatives);

    QAction* frameCurve = new ActionWithShortcut(kShortcutGroupCurveEditor,kShortcutIDActionCurveEditorCenter,
                                                 kShortcutDescActionCurveEditorCenter,interpMenu);
    frameCurve->setShortcut( QKeySequence(Qt::Key_F) );
    QObject::connect( frameCurve,SIGNAL( triggered() ),_widget,SLOT( frameSelectedCurve() ) );
    viewMenu->addAction(frameCurve);
    
    if (predefMenu) {
        QAction* loop = new QAction(QObject::tr("Loop"),_rightClickMenu);
        QObject::connect(loop, SIGNAL(triggered()), _widget, SLOT(loopSelectedCurve()));
        predefMenu->addAction(loop);
        
        QAction* reverse = new QAction(QObject::tr("Reverse"),_rightClickMenu);
        QObject::connect(reverse, SIGNAL(triggered()), _widget, SLOT(reverseSelectedCurve()));
        predefMenu->addAction(reverse);

        
        QAction* negate = new QAction(QObject::tr("Negate"),_rightClickMenu);
        QObject::connect(negate, SIGNAL(triggered()), _widget, SLOT(negateSelectedCurve()));
        predefMenu->addAction(negate);

    }

    QAction* updateOnPenUp = new QAction(QObject::tr("Update on mouse release only"),_rightClickMenu);
    updateOnPenUp->setCheckable(true);
    updateOnPenUp->setChecked( appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly() );
    _rightClickMenu->addAction(updateOnPenUp);
    QObject::connect( updateOnPenUp,SIGNAL( triggered() ),_widget,SLOT( onUpdateOnPenUpActionTriggered() ) );
} // createMenu

void
CurveWidgetPrivate::drawSelectionRectangle()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);

        glColor4f(0.3,0.3,0.3,0.2);
        QPointF btmRight = _selectionRectangle.bottomRight();
        QPointF topLeft = _selectionRectangle.topLeft();

        glBegin(GL_POLYGON);
        glVertex2f( topLeft.x(),btmRight.y() );
        glVertex2f( topLeft.x(),topLeft.y() );
        glVertex2f( btmRight.x(),topLeft.y() );
        glVertex2f( btmRight.x(),btmRight.y() );
        glEnd();


        glLineWidth(1.5);

        glColor4f(0.5,0.5,0.5,1.);
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
CurveWidgetPrivate::refreshTimelinePositions()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    
    
    QPointF topLeft = zoomCtx.toZoomCoordinates(0,0);
    QPointF btmRight = zoomCtx.toZoomCoordinates(_widget->width() - 1,_widget->height() - 1);
    QPointF btmCursorBtm( _timeline->currentFrame(),btmRight.y() );
    QPointF btmcursorBtmWidgetCoord = zoomCtx.toWidgetCoordinates( btmCursorBtm.x(),btmCursorBtm.y() );
    QPointF btmCursorTop = zoomCtx.toZoomCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - CURSOR_HEIGHT);
    QPointF btmCursorLeft = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() - CURSOR_WIDTH / 2., btmcursorBtmWidgetCoord.y() );
    QPointF btmCursorRight = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() + CURSOR_WIDTH / 2.,btmcursorBtmWidgetCoord.y() );
    QPointF topCursortop( _timeline->currentFrame(),topLeft.y() );
    QPointF topcursorTopWidgetCoord = zoomCtx.toWidgetCoordinates( topCursortop.x(),topCursortop.y() );
    QPointF topCursorBtm = zoomCtx.toZoomCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + CURSOR_HEIGHT);
    QPointF topCursorLeft = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() - CURSOR_WIDTH / 2., topcursorTopWidgetCoord.y() );
    QPointF topCursorRight = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() + CURSOR_WIDTH / 2.,topcursorTopWidgetCoord.y() );

    _timelineBtmPoly.clear();
    _timelineTopPoly.clear();

    _timelineBtmPoly.push_back(btmCursorTop);
    _timelineBtmPoly.push_back(btmCursorLeft);
    _timelineBtmPoly.push_back(btmCursorRight);

    _timelineTopPoly.push_back(topCursorBtm);
    _timelineTopPoly.push_back(topCursorLeft);
    _timelineTopPoly.push_back(topCursorRight);
}

void
CurveWidgetPrivate::drawTimelineMarkers()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );
    glCheckError();

    refreshTimelinePositions();
    
    double cursorR,cursorG,cursorB;
    double boundsR,boundsG,boundsB;
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    settings->getTimelinePlayheadColor(&cursorR, &cursorG, &cursorB);
    settings->getTimelineBoundsColor(&boundsR, &boundsG, &boundsB);

    QPointF topLeft = zoomCtx.toZoomCoordinates(0,0);
    QPointF btmRight = zoomCtx.toZoomCoordinates(_widget->width() - 1,_widget->height() - 1);

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
        glColor4f(boundsR,boundsG,boundsB,1.);

        double leftBound,rightBound;
        _gui->getApp()->getFrameRange(&leftBound, &rightBound);
        glBegin(GL_LINES);
        glVertex2f( leftBound,btmRight.y() );
        glVertex2f( leftBound,topLeft.y() );
        glVertex2f( rightBound,btmRight.y() );
        glVertex2f( rightBound,topLeft.y() );
        glColor4f(cursorR,cursorG,cursorB,1.);
        glVertex2f( _timeline->currentFrame(),btmRight.y() );
        glVertex2f( _timeline->currentFrame(),topLeft.y() );
        glEnd();
        glCheckErrorIgnoreOSXBug();

        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT,GL_DONT_CARE);

        assert(_timelineBtmPoly.size() == 3 && _timelineTopPoly.size() == 3);

        glBegin(GL_POLYGON);
        glVertex2f( _timelineBtmPoly.at(0).x(),_timelineBtmPoly.at(0).y() );
        glVertex2f( _timelineBtmPoly.at(1).x(),_timelineBtmPoly.at(1).y() );
        glVertex2f( _timelineBtmPoly.at(2).x(),_timelineBtmPoly.at(2).y() );
        glEnd();
        glCheckErrorIgnoreOSXBug();

        glBegin(GL_POLYGON);
        glVertex2f( _timelineTopPoly.at(0).x(),_timelineTopPoly.at(0).y() );
        glVertex2f( _timelineTopPoly.at(1).x(),_timelineTopPoly.at(1).y() );
        glVertex2f( _timelineTopPoly.at(2).x(),_timelineTopPoly.at(2).y() );
        glEnd();
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
    glCheckErrorIgnoreOSXBug();
}

void
CurveWidgetPrivate::drawCurves()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    //now draw each curve
    std::vector<boost::shared_ptr<CurveGui> > visibleCurves;
    _widget->getVisibleCurves(&visibleCurves);
    int count = (int)visibleCurves.size();

    for (int i = 0; i < count; ++i) {
        visibleCurves[i]->drawCurve(i, count);
    }
}


void
CurveWidgetPrivate::drawScale()
{
    glCheckError();
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    QPointF btmLeft = zoomCtx.toZoomCoordinates(0,_widget->height() - 1);
    QPointF topRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, 0);

    ///don't attempt to draw a scale on a widget with an invalid height/width
    if (_widget->height() <= 1 || _widget->width() <= 1) {
        return;
    }

    QFontMetrics fontM(*_font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    
    double gridR,gridG,gridB;
    boost::shared_ptr<Settings> sett = appPTR->getCurrentSettings();
    sett->getCurveEditorGridColor(&gridR, &gridG, &gridB);

    
    double scaleR,scaleG,scaleB;
    sett->getCurveEditorScaleColor(&scaleR, &scaleG, &scaleB);
    
    QColor scaleColor;
    scaleColor.setRgbF(Natron::clamp(scaleR, 0., 1.),
                       Natron::clamp(scaleG, 0., 1.),
                       Natron::clamp(scaleB, 0., 1.));

    
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
            const double minTickSizeTextPixel = (axis == 0) ? fontM.width( QString("00") ) : fontM.height(); // AXIS-SPECIFIC
            const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
            for (int i = m1; i <= m2; ++i) {
                double value = i * smallTickSize + offset;
                const double tickSize = ticks[i - m1] * smallTickSize;
                const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

                glCheckError();
                glColor4f(gridR,gridG,gridB, alpha);

                glBegin(GL_LINES);
                if (axis == 0) {
                    glVertex2f( value, btmLeft.y() ); // AXIS-SPECIFIC
                    glVertex2f( value, topRight.y() ); // AXIS-SPECIFIC
                } else {
                    glVertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                    glVertex2f(topRight.x(), value); // AXIS-SPECIFIC
                }
                glEnd();
                glCheckErrorIgnoreOSXBug();

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
                        QColor c = scaleColor;
                        c.setAlpha(255 * alphaText);
                        if (axis == 0) {
                            _widget->renderText(value, btmLeft.y(), s, c, *_font); // AXIS-SPECIFIC
                        } else {
                            _widget->renderText(btmLeft.x(), value, s, c, *_font); // AXIS-SPECIFIC
                        }
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
    
    
    glCheckError();
    glColor4f(gridR,gridG,gridB,1.);
    glBegin(GL_LINES);
    glVertex2f(AXIS_MIN, 0);
    glVertex2f(AXIS_MAX, 0);
    glVertex2f(0, AXIS_MIN);
    glVertex2f(0, AXIS_MAX);
    glEnd();

    
    glCheckErrorIgnoreOSXBug();
} // drawScale

void
CurveWidgetPrivate::drawSelectedKeyFramesBbox()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);

        QPointF topLeft = _selectedKeyFramesBbox.topLeft();
        QPointF btmRight = _selectedKeyFramesBbox.bottomRight();
        QPointF topLeftWidget = zoomCtx.toWidgetCoordinates(topLeft.x(), topLeft.y());
        QPointF btmRightWidget = zoomCtx.toWidgetCoordinates(btmRight.x(), btmRight.y());
        double xMid = (topLeft.x() + btmRight.x()) / 2.;
        double yMid = (topLeft.y() + btmRight.y()) / 2.;

        glLineWidth(1.5);

        glColor4f(0.5,0.5,0.5,1.);
        glBegin(GL_LINE_LOOP);
        glVertex2f( topLeft.x(),btmRight.y() );
        glVertex2f( topLeft.x(),topLeft.y() );
        glVertex2f( btmRight.x(),topLeft.y() );
        glVertex2f( btmRight.x(),btmRight.y() );
        glEnd();

        

        glBegin(GL_LINES);
        glVertex2f( std::max( _selectedKeyFramesCrossHorizLine.p1().x(),topLeft.x() ),_selectedKeyFramesCrossHorizLine.p1().y() );
        glVertex2f( std::min( _selectedKeyFramesCrossHorizLine.p2().x(),btmRight.x() ),_selectedKeyFramesCrossHorizLine.p2().y() );
        glVertex2f( _selectedKeyFramesCrossVertLine.p1().x(),std::max( _selectedKeyFramesCrossVertLine.p1().y(),btmRight.y() ) );
        glVertex2f( _selectedKeyFramesCrossVertLine.p2().x(),std::min( _selectedKeyFramesCrossVertLine.p2().y(),topLeft.y() ) );
        
        //top tick
        {
            double yBottom = zoomCtx.toZoomCoordinates(0, topLeftWidget.y() + BOUNDING_BOX_HANDLE_SIZE).y();
            double yTop = zoomCtx.toZoomCoordinates(0, topLeftWidget.y() - BOUNDING_BOX_HANDLE_SIZE).y();
            glVertex2f(xMid, yBottom);
            glVertex2f(xMid, yTop);
        }
        //left tick
        {
            double xLeft = zoomCtx.toZoomCoordinates(topLeftWidget.x() - BOUNDING_BOX_HANDLE_SIZE, 0).x();
            double xRight = zoomCtx.toZoomCoordinates(topLeftWidget.x() + BOUNDING_BOX_HANDLE_SIZE, 0).x();
            glVertex2f(xLeft, yMid);
            glVertex2f(xRight, yMid);
        }
        //bottom tick
        {
            double yBottom = zoomCtx.toZoomCoordinates(0, btmRightWidget.y() + BOUNDING_BOX_HANDLE_SIZE).y();
            double yTop = zoomCtx.toZoomCoordinates(0, btmRightWidget.y() - BOUNDING_BOX_HANDLE_SIZE).y();
            glVertex2f(xMid, yBottom);
            glVertex2f(xMid, yTop);
        }
        //right tick
        {
            double xLeft = zoomCtx.toZoomCoordinates(btmRightWidget.x() - BOUNDING_BOX_HANDLE_SIZE, 0).x();
            double xRight = zoomCtx.toZoomCoordinates(btmRightWidget.x() + BOUNDING_BOX_HANDLE_SIZE, 0).x();
            glVertex2f(xLeft, yMid);
            glVertex2f(xRight, yMid);
        }
        glEnd();
        
        glPointSize(BOUNDING_BOX_HANDLE_SIZE);
        glBegin(GL_POINTS);
        glVertex2f(topLeft.x(), topLeft.y());
        glVertex2f(btmRight.x(), topLeft.y());
        glVertex2f(btmRight.x(), btmRight.y());
        glVertex2f(topLeft.x(), btmRight.y());
        glEnd();
        
        glCheckError();
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
}

Curves::const_iterator
CurveWidgetPrivate::isNearbyCurve(const QPoint &pt,double* x,double *y) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF openGL_pos = zoomCtx.toZoomCoordinates( pt.x(),pt.y() );
    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            
            //Try once with expressions
            double yCurve;
            
            try {
                boost::shared_ptr<Curve> internalCurve = (*it)->getInternalCurve();
                yCurve = (*it)->evaluate(internalCurve && !internalCurve->isAnimated(), openGL_pos.x() );
            } catch (...) {
                yCurve = (*it)->evaluate(false, openGL_pos.x() );
            }
            
            double yWidget = zoomCtx.toWidgetCoordinates(0,yCurve).y();
            if (pt.y() < yWidget + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.y() > yWidget - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE ) {
                if (x != NULL) {
                    *x = openGL_pos.x();
                }
                if (y != NULL) {
                    *y = yCurve;
                }
                return it;
            }
            
        }
    }

    return _curves.end();
}

std::pair<boost::shared_ptr<CurveGui>,KeyFrame> CurveWidgetPrivate::isNearbyKeyFrame(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            KeyFrameSet keyFrames = (*it)->getKeyFrames();
            for (KeyFrameSet::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                QPointF keyFramewidgetPos = zoomCtx.toWidgetCoordinates( it2->getTime(), it2->getValue() );
                if ( (std::abs( pt.y() - keyFramewidgetPos.y() ) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                     (std::abs( pt.x() - keyFramewidgetPos.x() ) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) {
                    return std::make_pair(*it,*it2);
                }
            }
        }
    }

    return std::make_pair( (CurveGui*)NULL,KeyFrame() );
}

KeyPtr
CurveWidgetPrivate::isNearbyKeyFrameText(const QPoint& pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (_selectedKeyFrames.size() != 1) {
        return KeyPtr();
    }
    
    QFontMetrics fm(_widget->font());
    int yOffset = 4;
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        
        BezierCPCurveGui* isBezier = dynamic_cast<BezierCPCurveGui*>((*it)->curve.get());
        if (!isBezier) {
            QPointF topLeftWidget = zoomCtx.toWidgetCoordinates( (*it)->key.getTime(), (*it)->key.getValue() );
            topLeftWidget.ry() += yOffset;
            
            QString coordStr =  QString("x: %1, y: %2").arg((*it)->key.getTime()).arg((*it)->key.getValue());
            
            QPointF btmRightWidget(topLeftWidget.x() + fm.width(coordStr),topLeftWidget.y() + fm.height());
            
            if (pt.x() >= topLeftWidget.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.x() <= btmRightWidget.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE &&
                    pt.y() >= topLeftWidget.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.y() <= btmRightWidget.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) {
                return *it;
            }
        }
    }
    return KeyPtr();
}

std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr >
CurveWidgetPrivate::isNearbyTangent(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        BezierCPCurveGui* isBezier = dynamic_cast<BezierCPCurveGui*>((*it)->curve.get());
        if (!isBezier) {
            QPointF leftTanPt = zoomCtx.toWidgetCoordinates( (*it)->leftTan.first,(*it)->leftTan.second );
            QPointF rightTanPt = zoomCtx.toWidgetCoordinates( (*it)->rightTan.first,(*it)->rightTan.second );
            if ( ( pt.x() >= (leftTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                 ( pt.x() <= (leftTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                 ( pt.y() <= (leftTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                 ( pt.y() >= (leftTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
                return std::make_pair(MoveTangentCommand::eSelectedTangentLeft,*it);
            } else if ( ( pt.x() >= (rightTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                        ( pt.x() <= (rightTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                        ( pt.y() <= (rightTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                        ( pt.y() >= (rightTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
                return std::make_pair(MoveTangentCommand::eSelectedTangentRight,*it);
            }
        }
    }

    return std::make_pair( MoveTangentCommand::eSelectedTangentLeft,KeyPtr() );
}

std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr>
CurveWidgetPrivate::isNearbySelectedTangentText(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (_selectedKeyFrames.size() != 1) {
        return std::make_pair( MoveTangentCommand::eSelectedTangentLeft,KeyPtr() );
    }
    QFontMetrics fm(_widget->font());
    int yOffset = 4;
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        
        BezierCPCurveGui* isBezier = dynamic_cast<BezierCPCurveGui*>((*it)->curve.get());
        if (!isBezier) {
            double rounding = std::pow(10., CURVEWIDGET_DERIVATIVE_ROUND_PRECISION);
            
            QPointF topLeft_LeftTanWidget = zoomCtx.toWidgetCoordinates( (*it)->leftTan.first, (*it)->leftTan.second);
            QPointF topLeft_RightTanWidget = zoomCtx.toWidgetCoordinates( (*it)->rightTan.first, (*it)->rightTan.second);
            topLeft_LeftTanWidget.ry() += yOffset;
            topLeft_RightTanWidget.ry() += yOffset;
            
            QString leftCoordStr =  QString(QObject::tr("l: %1")).arg(std::floor(((*it)->key.getLeftDerivative() * rounding) + 0.5) / rounding);
            QString rightCoordStr =  QString(QObject::tr("r: %1")).arg(std::floor(((*it)->key.getRightDerivative() * rounding) + 0.5) / rounding);
            
            QPointF btmRight_LeftTanWidget(topLeft_LeftTanWidget.x() + fm.width(leftCoordStr),topLeft_LeftTanWidget.y() + fm.height());
            QPointF btmRight_RightTanWidget(topLeft_RightTanWidget.x() + fm.width(rightCoordStr),topLeft_RightTanWidget.y() + fm.height());
            
            if (pt.x() >= topLeft_LeftTanWidget.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.x() <= btmRight_LeftTanWidget.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE &&
                    pt.y() >= topLeft_LeftTanWidget.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.y() <= btmRight_LeftTanWidget.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) {
                return std::make_pair(MoveTangentCommand::eSelectedTangentLeft, *it);
            } else if (pt.x() >= topLeft_RightTanWidget.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.x() <= btmRight_RightTanWidget.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE &&
                       pt.y() >= topLeft_RightTanWidget.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE && pt.y() <= btmRight_RightTanWidget.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) {
                return std::make_pair(MoveTangentCommand::eSelectedTangentRight, *it);
            }
        }
    }
    return std::make_pair( MoveTangentCommand::eSelectedTangentLeft,KeyPtr() );
}

bool
CurveWidgetPrivate::isNearbySelectedKeyFramesCrossWidget(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF middleLeft = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossHorizLine.p1().x(),_selectedKeyFramesCrossHorizLine.p1().y() );
    QPointF middleRight = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossHorizLine.p2().x(),_selectedKeyFramesCrossHorizLine.p2().y() );
    QPointF middleBtm = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossVertLine.p1().x(),_selectedKeyFramesCrossVertLine.p1().y() );
    QPointF middleTop = zoomCtx.toWidgetCoordinates( _selectedKeyFramesCrossVertLine.p2().x(),_selectedKeyFramesCrossVertLine.p2().y() );

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
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x(), _selectedKeyFramesBbox.y());
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
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x(), _selectedKeyFramesBbox.y() +
                                                _selectedKeyFramesBbox.height());
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
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width() / 2., _selectedKeyFramesBbox.y() +
                                                _selectedKeyFramesBbox.height());
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
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width(), _selectedKeyFramesBbox.y() +
                                                _selectedKeyFramesBbox.height());
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
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width(), _selectedKeyFramesBbox.y());
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
    QPointF other = zoomCtx.toWidgetCoordinates(_selectedKeyFramesBbox.x() + _selectedKeyFramesBbox.width() / 2., _selectedKeyFramesBbox.y());
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

    QPointF pt_opengl = zoomCtx.toZoomCoordinates( pt.x(),pt.y() );

    return _timelineTopPoly.containsPoint(pt_opengl,Qt::OddEvenFill);
}

bool
CurveWidgetPrivate::isNearbyTimelineBtmPoly(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF pt_opengl = zoomCtx.toZoomCoordinates( pt.x(),pt.y() );

    return _timelineBtmPoly.containsPoint(pt_opengl,Qt::OddEvenFill);
}

/**
 * @brief Selects the curve given in parameter and deselects any other curve in the widget.
 **/
void
CurveWidgetPrivate::selectCurve(const boost::shared_ptr<CurveGui>& curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!curve) {
        return;
    }
    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        (*it)->setSelected(false);
    }
    curve->setSelected(true);
    
    if ( _widget->parentWidget() ) {
        QWidget* parent  = _widget->parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == "CurveEditor") {
                CurveEditor* ce = dynamic_cast<CurveEditor*>(parent);
                if (ce) {
                    ce->setSelectedCurve(curve);
                }
            }
        }
    }
}

void
CurveWidgetPrivate::keyFramesWithinRect(const QRectF & rect,
                                        std::vector< std::pair<boost::shared_ptr<CurveGui>,KeyFrame > >* keys) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double left = rect.topLeft().x();
    double right = rect.bottomRight().x();
    double bottom = rect.topLeft().y();
    double top = rect.bottomRight().y();
    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            
            KeyFrameSet set = (*it)->getKeyFrames();
            for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
                double y = it2->getValue();
                double x = it2->getTime();
                if ( (x <= right) && (x >= left) && (y <= top) && (y >= bottom) ) {
                    keys->push_back( std::make_pair(*it,*it2) );
                }
            }

        }
    }
}


namespace  {
struct SortIncreasingFunctor {
    
    bool operator() (const KeyPtr& lhs,const KeyPtr& rhs) {
        if (lhs->curve.get() < rhs->curve.get()) {
            return true;
        } else if (lhs->curve.get() > rhs->curve.get()) {
            return false;
        } else {
            return lhs->key.getTime() < rhs->key.getTime();
        }
    }
};

struct SortDecreasingFunctor {
    
    bool operator() (const KeyPtr& lhs,const KeyPtr& rhs) {
        if (lhs->curve.get() < rhs->curve.get()) {
            return true;
        } else if (lhs->curve.get() > rhs->curve.get()) {
            return false;
        } else {
            return lhs->key.getTime() > rhs->key.getTime();
        }
    }
};
}


void
CurveWidgetPrivate::moveSelectedKeyFrames(const QPointF & oldClick_opengl,
                                          const QPointF & newClick_opengl)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF dragStartPointOpenGL = zoomCtx.toZoomCoordinates( _dragStartPoint.x(),_dragStartPoint.y() );
    bool clampToIntegers = ( *_selectedKeyFrames.begin() )->curve->areKeyFramesTimeClampedToIntegers();
    
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
            if ( (*it)->curve->areKeyFramesValuesClampedToBooleans() ) {
                totalMovement.ry() = std::max( 0.,std::min(std::floor(totalMovement.y() + 0.5),1.) );
                break;
            } else if ( (*it)->curve->areKeyFramesValuesClampedToIntegers() ) {
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
    std::vector<KeyPtr> vect;
    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        boost::shared_ptr<Curve> curve = (*it)->curve->getInternalCurve();
        if (curve) {
            KeyFrame prevKey,nextKey;
            if (curve->getNextKeyframeTime((*it)->key.getTime(), &nextKey)) {
                if (!_widget->isSelectedKey((*it)->curve, nextKey.getTime())) {
                    double diff = nextKey.getTime() - (*it)->key.getTime() - epsilon;
                    maxRight = std::max(0.,std::min(diff, maxRight));
                }
            }
            if (curve->getPreviousKeyframeTime((*it)->key.getTime(), &prevKey)) {
                if (!_widget->isSelectedKey((*it)->curve, prevKey.getTime())) {
                    double diff = prevKey.getTime()  - (*it)->key.getTime() + epsilon;
                    maxLeft = std::min(0.,std::max(diff, maxLeft));
                }
            }
        } else {
            BezierCPCurveGui* bezierCurve = dynamic_cast<BezierCPCurveGui*>((*it)->curve.get());
            assert(bezierCurve);
            std::set<int> keyframes;
            bezierCurve->getBezier()->getKeyframeTimes(&keyframes);
            std::set<int>::iterator found = keyframes.find((*it)->key.getTime());
            assert(found != keyframes.end());
            if (found != keyframes.begin()) {
                std::set<int>::iterator prev = found;
                --prev;
                if (!_widget->isSelectedKey((*it)->curve, *prev)) {
                    double diff = *prev  - *found + epsilon;
                    maxLeft = std::min(0.,std::max(diff, maxLeft));
                }
            }
            if (found != keyframes.end()) {
                std::set<int>::iterator next = found;
                ++next;
                if (!_widget->isSelectedKey((*it)->curve, *next)) {
                    double diff = *next - *found - epsilon;
                    maxRight = std::max(0.,std::min(diff, maxRight));
                }
            }
        }
        vect.push_back(*it);
    }
    
    dt = std::min(dt, maxRight);
    dt = std::max(dt, maxLeft);
    
    //Keyframes must be sorted in order according to the user movement otherwise if keyframes are next to each other we might override
    //another keyframe.
    //Can only call sort on random iterators
    if (dt < 0) {
        std::sort(vect.begin(), vect.end(), SortIncreasingFunctor());
    } else {
        std::sort(vect.begin(), vect.end(), SortDecreasingFunctor());
    }
    
    double dv;
    ///Parametric curve editor (the ones of the KnobParametric) never clamp keyframes to integer in the X direction
    ///We also want them to allow the user to move freely the keyframes around, hence the !clampToIntegers
    if ( (_mouseDragOrientation.y() != 0) || !useOneDirectionOnly ) {
        if (!useOneDirectionOnly) {
            dv = totalMovement.y();
        } else {
            dv = totalMovement.y() - _keyDragLastMovement.y();
        }
    } else {
        dv = 0;
    }

    if ( (dt != 0) || (dv != 0) ) {
        
        SelectedKeys keysInOrder;
        for (std::vector<KeyPtr>::const_iterator it = vect.begin(); it != vect.end(); ++it) {
            if ( !(*it)->curve->isYComponentMovable() ) {
                dv = 0;
            }
            keysInOrder.push_back(*it);
        }

        if ( (dt != 0) || (dv != 0) ) {
            bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
            _widget->pushUndoCommand( new MoveKeysCommand(_widget,keysInOrder,dt,dv,!updateOnPenUpOnly) );
            _evaluateOnPenUp = true;
        }
    }
    //update last drag movement
    if ( (_mouseDragOrientation.x() != 0) || !useOneDirectionOnly ) {
        _keyDragLastMovement.rx() = totalMovement.x();
    }
    if (_mouseDragOrientation.y() != 0  || !useOneDirectionOnly ) {
        _keyDragLastMovement.ry() = totalMovement.y();
    }
} // moveSelectedKeyFrames

void
CurveWidgetPrivate::transformSelectedKeyFrames(const QPointF & oldClick_opengl,const QPointF & newClick_opengl, bool shiftHeld)
{
    if (newClick_opengl == oldClick_opengl) {
        return;
    }
    
    QPointF dragStartPointOpenGL = zoomCtx.toZoomCoordinates( _dragStartPoint.x(),_dragStartPoint.y() );
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
    
    QPointF totalMovement(newClick_opengl.x() - dragStartPointOpenGL.x(), newClick_opengl.y() - dragStartPointOpenGL.y());

    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        if ( (*it)->curve->areKeyFramesValuesClampedToBooleans() ) {
            totalMovement.ry() = std::max( 0.,std::min(std::floor(totalMovement.y() + 0.5),1.) );
            break;
        } else if ( (*it)->curve->areKeyFramesValuesClampedToIntegers() ) {
            totalMovement.ry() = std::floor(totalMovement.y() + 0.5);
        }
    }
    
    
    double dt;
    dt = totalMovement.x() - _keyDragLastMovement.x();
    
    double dv;
    dv = totalMovement.y() - _keyDragLastMovement.y();
    
    
    if (dt != 0 || dv != 0) {
        QPointF center = _selectedKeyFramesBbox.center();
        
        if (shiftHeld &&
                (_state == eEventStateDraggingMidRightBbox ||
                 _state == eEventStateDraggingMidBtmBbox ||
                 _state == eEventStateDraggingMidLeftBbox ||
                 _state == eEventStateDraggingMidTopBbox)) {
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
        
        double sx = 1.,sy = 1.;
        double tx = 0., ty = 0.;
        double oldX = newClick_opengl.x() - dt;
        double oldY = newClick_opengl.y() - dv;
        // the scale ratio is the ratio of distances to the center
        double prevDist = ( oldX - center.x() ) * ( oldX - center.x() ) + ( oldY - center.y() ) * ( oldY - center.y() );
        
        if (prevDist != 0) {
            double dist = ( newClick_opengl.x() - center.x() ) * ( newClick_opengl.x() - center.x() ) + ( newClick_opengl.y() - center.y() ) * ( newClick_opengl.y() - center.y() );
            double ratio = std::sqrt(dist / prevDist);
            
            if (_state == eEventStateDraggingBtmLeftBbox ||
                    _state == eEventStateDraggingBtmRightBbox ||
                    _state == eEventStateDraggingTopRightBbox ||
                    _state == eEventStateDraggingTopLeftBbox) {
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
        _widget->pushUndoCommand( new TransformKeysCommand(_widget,_selectedKeyFrames,center.x(),center.y(),tx,ty,sx,sy,!updateOnPenUpOnly) );
        _evaluateOnPenUp = true;
    }
    //update last drag movement
    if (dt != 0) {
        _keyDragLastMovement.rx() = totalMovement.x();
    }
    
    if (dv != 0) {
        _keyDragLastMovement.ry() = totalMovement.y();
    }
    

}


void
CurveWidgetPrivate::moveSelectedTangent(const QPointF & pos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // the key should exist
    assert( std::find(_selectedKeyFrames.begin(),_selectedKeyFrames.end(),_selectedDerivative.second) != _selectedKeyFrames.end() );

    KeyPtr & key = _selectedDerivative.second;
    
    double dy = key->key.getValue() - pos.y();
    double dx = key->key.getTime() - pos.x();
    
    
    ///If the knob is evaluating on change and _updateOnPenUpOnly is true, set it to false prior
    ///to modifying the keyframe, and set it back to its original value afterwards.
    
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
    
    _widget->pushUndoCommand(new MoveTangentCommand(_widget,_selectedDerivative.first,key,dx,dy,!updateOnPenUpOnly));

} // moveSelectedTangent

void
CurveWidgetPrivate::refreshKeyTangents(KeyPtr & key)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double w = (double)_widget->width();
    double h = (double)_widget->height();
    double x = key->key.getTime();
    double y = key->key.getValue();
    QPointF keyWidgetCoord = zoomCtx.toWidgetCoordinates(x,y);
    KeyFrameSet keyframes = key->curve->getKeyFrames();
    KeyFrameSet::const_iterator k = keyframes.find(key->key);

    //the key might have disappeared from the curve if the plugin deleted it.
    //In which case we return.
    if ( k == keyframes.end() ) {
        return;
    }

    //find the previous and next keyframes on the curve to find out the  position of the derivatives
    KeyFrameSet::const_iterator prev = k;
    if ( k != keyframes.begin() ) {
        --prev;
    } else {
        prev = keyframes.end();
    }
    KeyFrameSet::const_iterator next = k;
    if (next != keyframes.end()) {
        ++next;
    }
    double leftTanX, leftTanY;
    {
        double prevTime = ( prev == keyframes.end() ) ? (x - 1.) : prev->getTime();
        double leftTan = key->key.getLeftDerivative();
        double leftTanXWidgetDiffMax = w / 8.;
        if ( prev != keyframes.end() ) {
            double prevKeyXWidgetCoord = zoomCtx.toWidgetCoordinates(prevTime, 0).x();
            //set the left derivative X to be at 1/3 of the interval [prev,k], and clamp it to 1/8 of the widget width.
            leftTanXWidgetDiffMax = std::min(leftTanXWidgetDiffMax, (keyWidgetCoord.x() - prevKeyXWidgetCoord) / 3.);
        }
        //clamp the left derivative Y to 1/8 of the widget height.
        double leftTanYWidgetDiffMax = std::min( h / 8., leftTanXWidgetDiffMax);
        assert(leftTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(leftTanYWidgetDiffMax >= 0.);

        QPointF tanMax = zoomCtx.toZoomCoordinates(keyWidgetCoord.x() + leftTanXWidgetDiffMax, keyWidgetCoord.y() - leftTanYWidgetDiffMax) - QPointF(x,y);
        assert(tanMax.x() >= 0.); // both should be positive
        assert(tanMax.y() >= 0.);

        if ( tanMax.x() * std::abs(leftTan) < tanMax.y() ) {
            leftTanX = x - tanMax.x();
            leftTanY = y - tanMax.x() * leftTan;
        } else {
            leftTanX = x - tanMax.y() / std::abs(leftTan);
            leftTanY = y - tanMax.y() * (leftTan > 0 ? 1 : -1);
        }
        assert(std::abs(leftTanX - x) <= tanMax.x() * 1.001); // check that they are effectively clamped (taking into account rounding errors)
        assert(std::abs(leftTanY - y) <= tanMax.y() * 1.001);
    }
    double rightTanX, rightTanY;
    {
        double nextTime = ( next == keyframes.end() ) ? (x + 1.) : next->getTime();
        double rightTan = key->key.getRightDerivative();
        double rightTanXWidgetDiffMax = w / 8.;
        if ( next != keyframes.end() ) {
            double nextKeyXWidgetCoord = zoomCtx.toWidgetCoordinates(nextTime, 0).x();
            //set the right derivative X to be at 1/3 of the interval [k,next], and clamp it to 1/8 of the widget width.
            rightTanXWidgetDiffMax = std::min(rightTanXWidgetDiffMax, ( nextKeyXWidgetCoord - keyWidgetCoord.x() ) / 3.);
        }
        //clamp the right derivative Y to 1/8 of the widget height.
        double rightTanYWidgetDiffMax = std::min( h / 8., rightTanXWidgetDiffMax);
        assert(rightTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(rightTanYWidgetDiffMax >= 0.);

        QPointF tanMax = zoomCtx.toZoomCoordinates(keyWidgetCoord.x() + rightTanXWidgetDiffMax, keyWidgetCoord.y() - rightTanYWidgetDiffMax) - QPointF(x,y);
        assert(tanMax.x() >= 0.); // both bounds should be positive
        assert(tanMax.y() >= 0.);

        if ( tanMax.x() * std::abs(rightTan) < tanMax.y() ) {
            rightTanX = x + tanMax.x();
            rightTanY = y + tanMax.x() * rightTan;
        } else {
            rightTanX = x + tanMax.y() / std::abs(rightTan);
            rightTanY = y + tanMax.y() * (rightTan > 0 ? 1 : -1);
        }
        assert(std::abs(rightTanX - x) <= tanMax.x() * 1.001); // check that they are effectively clamped (taking into account rounding errors)
        assert(std::abs(rightTanY - y) <= tanMax.y() * 1.001);
    }
    key->leftTan.first = leftTanX;
    key->leftTan.second = leftTanY;
    key->rightTan.first = rightTanX;
    key->rightTan.second = rightTanY;
} // refreshKeyTangents

void
CurveWidgetPrivate::refreshSelectionRectangle(double x,
                                              double y)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double xmin = std::min(_dragStartPoint.x(),x);
    double xmax = std::max(_dragStartPoint.x(),x);
    double ymin = std::min(_dragStartPoint.y(),y);
    double ymax = std::max(_dragStartPoint.y(),y);
    _selectionRectangle.setBottomRight( zoomCtx.toZoomCoordinates(xmax,ymin) );
    _selectionRectangle.setTopLeft( zoomCtx.toZoomCoordinates(xmin,ymax) );
    _selectedKeyFrames.clear();
    std::vector< std::pair<boost::shared_ptr<CurveGui>,KeyFrame > > keyframesSelected;
    keyFramesWithinRect(_selectionRectangle,&keyframesSelected);
    for (U32 i = 0; i < keyframesSelected.size(); ++i) {
        KeyPtr newSelectedKey( new SelectedKey(keyframesSelected[i].first,keyframesSelected[i].second) );
        refreshKeyTangents(newSelectedKey);
        //insert it into the _selectedKeyFrames
        _selectedKeyFrames.push_back(newSelectedKey);
    }

    _widget->refreshSelectedKeysBbox();
}



void
CurveWidgetPrivate::setSelectedKeysInterpolation(Natron::KeyframeTypeEnum type)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::list<KeyInterpolationChange> changes;
    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        KeyInterpolationChange change( (*it)->key.getInterpolation(),type,(*it) );
        changes.push_back(change);
    }
    _widget->pushUndoCommand( new SetKeysInterpolationCommand(_widget,changes) );
}

void
CurveWidgetPrivate::insertSelectedKeyFrameConditionnaly(const KeyPtr &key)
{
    //insert it into the _selectedKeyFrames
    bool alreadySelected = false;

    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        if ( ( (*it)->curve == key->curve ) && ( (*it)->key.getTime() == key->key.getTime() ) ) {
            ///key is already selected
            alreadySelected = true;
            break;
        }
    }

    if (!alreadySelected) {
        _selectedKeyFrames.push_back(key);
    }
}

void
CurveWidgetPrivate::updateDopeSheetViewFrameRange()
{
    _gui->getDopeSheetEditor()->centerOn(zoomCtx.left(), zoomCtx.right());
}



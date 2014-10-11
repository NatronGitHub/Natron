//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "CurveWidget.h"

#include <QMenu>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtCore/QRectF>
#include <QtGui/QPolygonF>
#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QVBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QTextStream>
#include <QThread>
#include <QUndoStack>
#include <QApplication>

#include "Engine/Knob.h"
#include "Engine/Rect.h"
#include "Engine/TimeLine.h"
#include "Engine/Variant.h"
#include "Engine/Curve.h"
#include "Engine/Settings.h"

#include "Gui/LineEdit.h"
#include "Gui/SpinBox.h"
#include "Gui/Button.h"
#include "Gui/ticks.h"
#include "Gui/CurveEditor.h"
#include "Gui/TextRenderer.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/KnobGui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/ZoomContext.h"
#include "Gui/TabWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ViewerGL.h"
#include "Gui/NodeGraph.h"
#include "Gui/Histogram.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

using namespace Natron;

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
                                               // (in widget pixels)
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

static double AXIS_MAX = 100000.;
static double AXIS_MIN = -100000.;


namespace { // protect local classes in anonymous namespace
enum EventState
{
    DRAGGING_VIEW = 0,
    DRAGGING_KEYS = 1,
    SELECTING = 2,
    DRAGGING_TANGENT = 3,
    DRAGGING_TIMELINE = 4,
    NONE = 5
};

struct SelectedKey_belongs_to_curve
{
    const CurveGui* _curve;
    SelectedKey_belongs_to_curve(const CurveGui* curve)
        : _curve(curve)
    {
    }

    bool operator() (const SelectedKey & k) const
    {
        return k.curve == _curve;
    }
};
}

CurveGui::CurveGui(const CurveWidget *curveWidget,
                   boost::shared_ptr<Curve> curve,
                   KnobGui* knob,
                   int dimension,
                   const QString & name,
                   const QColor & color,
                   int thickness)
    : _internalCurve(curve)
      , _name(name)
      , _color(color)
      , _thickness(thickness)
      , _visible(false)
      , _selected(false)
      , _curveWidget(curveWidget)
      , _knob(knob)
      , _dimension(dimension)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // even when there is only one keyframe, there may be tangents!
    if (curve->getKeyFramesCount() > 0) {
        _visible = true;
    }

    QObject::connect( this,SIGNAL( curveChanged() ),curveWidget,SLOT( onCurveChanged() ) );
}

CurveGui::~CurveGui()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

std::pair<KeyFrame,bool> CurveGui::nextPointForSegment(double x1,
                                                       double* x2,
                                                       const KeyFrameSet & keys)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( !keys.empty() );
    double xminCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keys.begin()->getTime(),0).x();
    double xmaxCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keys.rbegin()->getTime(),0).x();
    std::pair<double,double> curveYRange;
    try {
        curveYRange = _internalCurve->getCurveYRange();
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;

        return std::make_pair(KeyFrame(0.,0.),false);
    }

    if (x1 < xminCurveWidgetCoord) {
        if ( ( curveYRange.first <= kOfxFlagInfiniteMin) && ( curveYRange.second >= kOfxFlagInfiniteMax) ) {
            *x2 = xminCurveWidgetCoord;
        } else {
            ///the curve has a min/max, find out the slope of the curve so we know whether the curve intersects
            ///the min axis, the max axis or nothing.
            if (keys.size() == 1) {
                ///if only 1 keyframe, the curve is horizontal
                *x2 = xminCurveWidgetCoord;
            } else {
                ///find out the equation of the straight line going from the first keyframe and intersecting
                ///the min axis, so we can get the coordinates of the point intersecting the min axis.
                KeyFrameSet::const_iterator firstKf = keys.begin();

                if (firstKf->getLeftDerivative() == 0) {
                    *x2 = xminCurveWidgetCoord;
                } else {
                    double b = firstKf->getValue() - firstKf->getLeftDerivative() * firstKf->getTime();
                    *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.first - b) / firstKf->getLeftDerivative(),0 ).x();
                    if ( (x1 >= *x2) || (*x2 > xminCurveWidgetCoord) ) {
                        ///do the same wit hthe max axis
                        *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.second - b) / firstKf->getLeftDerivative(),0 ).x();

                        if ( (x1 >= *x2) || (*x2 > xminCurveWidgetCoord) ) {
                            /// ok the curve doesn't intersect the min/max axis
                            *x2 = xminCurveWidgetCoord;
                        }
                    }
                }
            }
        }
    } else if (x1 >= xmaxCurveWidgetCoord) {
        if ( (curveYRange.first <= kOfxFlagInfiniteMin) && (curveYRange.second >= kOfxFlagInfiniteMax) ) {
            *x2 = _curveWidget->width() - 1;
        } else {
            ///the curve has a min/max, find out the slope of the curve so we know whether the curve intersects
            ///the min axis, the max axis or nothing.
            if (keys.size() == 1) {
                ///if only 1 keyframe, the curve is horizontal
                *x2 = _curveWidget->width() - 1;
            } else {
                ///find out the equation of the straight line going from the last keyframe and intersecting
                ///the min axis, so we can get the coordinates of the point intersecting the min axis.
                KeyFrameSet::const_reverse_iterator lastKf = keys.rbegin();

                if (lastKf->getRightDerivative() == 0) {
                    *x2 = _curveWidget->width() - 1;
                } else {
                    double b = lastKf->getValue() - lastKf->getRightDerivative() * lastKf->getTime();
                    *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.first - b) / lastKf->getRightDerivative(),0 ).x();
                    if ( (x1 >= *x2) || (*x2 < xmaxCurveWidgetCoord) ) {
                        ///do the same wit hthe min axis
                        *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.second - b) / lastKf->getRightDerivative(),0 ).x();

                        if ( (x1 >= *x2) || (*x2 < xmaxCurveWidgetCoord) ) {
                            /// ok the curve doesn't intersect the min/max axis
                            *x2 = _curveWidget->width() - 1;
                        }
                    }
                }
            }
        }
    } else {
        //we're between 2 keyframes,get the upper and lower
        KeyFrameSet::const_iterator upper = keys.end();
        double upperWidgetCoord = x1;
        for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            upperWidgetCoord = _curveWidget->toWidgetCoordinates(it->getTime(),0).x();
            if (upperWidgetCoord > x1) {
                upper = it;
                break;
            }
        }
        assert( upper != keys.end() && upper != keys.begin() );

        KeyFrameSet::const_iterator lower = upper;
        --lower;

        double t = ( x1 - lower->getTime() ) / ( upper->getTime() - lower->getTime() );
        double P3 = upper->getValue();
        double P0 = lower->getValue();
        // Hermite coefficients P0' and P3' are for t normalized in [0,1]
        double P3pl = upper->getLeftDerivative() / ( upper->getTime() - lower->getTime() ); // normalize for t \in [0,1]
        double P0pr = lower->getRightDerivative() / ( upper->getTime() - lower->getTime() ); // normalize for t \in [0,1]
        double secondDer = 6. * (1. - t) * (P3 - P3pl / 3. - P0 - 2. * P0pr / 3.) +
                           6. * t * (P0 - P3 + 2 * P3pl / 3. + P0pr / 3. );
        double secondDerWidgetCoord = std::abs( _curveWidget->toWidgetCoordinates(0,secondDer).y()
                                                / ( upper->getTime() - lower->getTime() ) );
        // compute delta_x so that the y difference between the derivative and the curve is at most
        // 1 pixel (use the second order Taylor expansion of the function)
        double delta_x = std::max(2. / std::max(std::sqrt(secondDerWidgetCoord),0.1), 1.);

        if (upperWidgetCoord < x1 + delta_x) {
            *x2 = upperWidgetCoord;

            return std::make_pair(*upper,true);
        } else {
            *x2 = x1 + delta_x;
        }
    }

    return std::make_pair(KeyFrame(0.,0.),false);
} // nextPointForSegment

void
CurveGui::drawCurve(int curveIndex,
                    int curvesCount)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (!_visible) {
        return;
    }

    assert( QGLContext::currentContext() == _curveWidget->context() );

    std::vector<float> vertices;
    double x1 = 0;
    double x2;
    double w = _curveWidget->width();
    KeyFrameSet keyframes = _internalCurve->getKeyFrames_mt_safe();
    std::pair<KeyFrame,bool> isX1AKey;
    while ( x1 < (w - 1) ) {
        double x,y;
        if (!isX1AKey.second) {
            x = _curveWidget->toZoomCoordinates(x1,0).x();
            y = evaluate(x);
        } else {
            x = isX1AKey.first.getTime();
            y = isX1AKey.first.getValue();
        }
        vertices.push_back( (float)x );
        vertices.push_back( (float)y );
        isX1AKey = nextPointForSegment(x1,&x2,keyframes);
        x1 = x2;
    }
    //also add the last point
    {
        double x = _curveWidget->toZoomCoordinates(x1,0).x();
        double y = evaluate(x);
        vertices.push_back( (float)x );
        vertices.push_back( (float)y );
    }

    const QColor & curveColor = _selected ?  _curveWidget->getSelectedCurveColor() : _color;

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

        glColor4f( curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF() );
        glPointSize(_thickness);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
        glLineWidth(1.5);

        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < (int)vertices.size(); i += 2) {
            glVertex2f(vertices[i],vertices[i + 1]);
        }
        glEnd();


        //render the name of the curve
        glColor4f(1.f, 1.f, 1.f, 1.f);


        QPointF btmLeft = _curveWidget->toZoomCoordinates(0,_curveWidget->height() - 1);
        QPointF topRight = _curveWidget->toZoomCoordinates(_curveWidget->width() - 1, 0);
        double interval = ( topRight.x() - btmLeft.x() ) / (double)curvesCount;
        double textX = _curveWidget->toZoomCoordinates(15, 0).x() + interval * (double)curveIndex;
        double textY = evaluate(textX);

        _curveWidget->renderText( textX,textY,_name,_color,_curveWidget->getFont() );
        glColor4f( curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF() );


        //draw keyframes
        glPointSize(7.f);
        glEnable(GL_POINT_SMOOTH);

        const SelectedKeys & selectedKeyFrames = _curveWidget->getSelectedKeyFrames();
        for (KeyFrameSet::const_iterator k = keyframes.begin(); k != keyframes.end(); ++k) {
            glColor4f( _color.redF(), _color.greenF(), _color.blueF(), _color.alphaF() );
            const KeyFrame & key = (*k);
            //if the key is selected change its color to white
            SelectedKeys::const_iterator isSelected = selectedKeyFrames.end();
            for (SelectedKeys::const_iterator it2 = selectedKeyFrames.begin();
                 it2 != selectedKeyFrames.end(); ++it2) {
                if ( ( (*it2)->key.getTime() == key.getTime() ) && ( (*it2)->curve == this ) ) {
                    isSelected = it2;
                    glColor4f(1.f,1.f,1.f,1.f);
                    break;
                }
            }
            double x = key.getTime();
            double y = key.getValue();
            glBegin(GL_POINTS);
            glVertex2f(x,y);
            glEnd();
            if ( ( isSelected != selectedKeyFrames.end() ) && (key.getInterpolation() != KEYFRAME_CONSTANT) ) {
                //draw the derivatives lines
                if ( (key.getInterpolation() != KEYFRAME_FREE) && (key.getInterpolation() != KEYFRAME_BROKEN) ) {
                    glLineStipple(2, 0xAAAA);
                    glEnable(GL_LINE_STIPPLE);
                }
                glBegin(GL_LINES);
                glColor4f(1., 0.35, 0.35, 1.);
                glVertex2f( (*isSelected)->leftTan.first, (*isSelected)->leftTan.second );
                glVertex2f(x, y);
                glVertex2f(x, y);
                glVertex2f( (*isSelected)->rightTan.first, (*isSelected)->rightTan.second );
                glEnd();
                if ( (key.getInterpolation() != KEYFRAME_FREE) && (key.getInterpolation() != KEYFRAME_BROKEN) ) {
                    glDisable(GL_LINE_STIPPLE);
                }

                if (selectedKeyFrames.size() == 1) { //if one keyframe, also draw the coordinates
                    QString coordStr("x: %1, y: %2");
                    coordStr = coordStr.arg(x).arg(y);
                    double yWidgetCoord = _curveWidget->toWidgetCoordinates( 0,key.getValue() ).y();
                    QFontMetrics m( _curveWidget->getFont() );
                    yWidgetCoord += (m.height() + 4);
                    glColor4f(1., 1., 1., 1.);
                    glCheckFramebufferError();
                    _curveWidget->renderText( x, _curveWidget->toZoomCoordinates(0, yWidgetCoord).y(),
                                             coordStr, QColor(240,240,240), _curveWidget->getFont() );
                }
                glBegin(GL_POINTS);
                glVertex2f( (*isSelected)->leftTan.first, (*isSelected)->leftTan.second );
                glVertex2f( (*isSelected)->rightTan.first, (*isSelected)->rightTan.second );
                glEnd();
            }
        }
    } // GLProtectAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

    glCheckError();
} // drawCurve

double
CurveGui::evaluate(double x) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _internalCurve->getValueAt(x);
}

void
CurveGui::setVisible(bool visible)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _visible = visible;
}

void
CurveGui::setVisibleAndRefresh(bool visible)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _visible = visible;
    emit curveChanged();
}

/*****************************CURVE WIDGET***********************************************/

namespace { // protext local classes in anonymous namespace
typedef std::list<CurveGui* > Curves;
struct MaxMovement
{
    double left,right,bottom,top;
};
}

// although all members are public, CurveWidgetPrivate is really a class because it has lots of member functions
// (in fact, many members could probably be made private)
class CurveWidgetPrivate
{
public:

    CurveWidgetPrivate(Gui* gui,
                       boost::shared_ptr<TimeLine> timeline,
                       CurveWidget* widget);

    ~CurveWidgetPrivate();

    void drawSelectionRectangle();

    void refreshTimelinePositions();

    void drawTimelineMarkers();

    void drawCurves();

    void drawBaseAxis();

    void drawScale();
    void drawSelectedKeyFramesBbox();

    /**
     * @brief Returns whether the click at position pt is nearby the curve.
     * If so then the value x and y will be set to the position on the curve
     * if they are not NULL.
     **/
    Curves::const_iterator isNearbyCurve(const QPoint &pt,double* x = NULL,double *y = NULL) const;
    std::pair<CurveGui*,KeyFrame> isNearbyKeyFrame(const QPoint & pt) const;
    std::pair<CurveGui::SelectedDerivative, KeyPtr> isNearbyTangent(const QPoint & pt) const;

    bool isNearbySelectedKeyFramesCrossWidget(const QPoint & pt) const;

    bool isNearbyTimelineTopPoly(const QPoint & pt) const;

    bool isNearbyTimelineBtmPoly(const QPoint & pt) const;

    /**
     * @brief Selects the curve given in parameter and deselects any other curve in the widget.
     **/
    void selectCurve(CurveGui* curve);

    void moveSelectedKeyFrames(const QPointF & oldClick_opengl,const QPointF & newClick_opengl);

    void moveSelectedTangent(const QPointF & pos);

    void refreshKeyTangents(KeyPtr & key);

    void refreshSelectionRectangle(double x,double y);

    void updateSelectedKeysMaxMovement();

    void setSelectedKeysInterpolation(Natron::KeyframeType type);

    void createMenu();

    void insertSelectedKeyFrameConditionnaly(const KeyPtr & key);

    void pushUndoCommand(QUndoCommand* cmd);

private:


    void keyFramesWithinRect(const QRectF & rect,std::vector< std::pair<CurveGui*,KeyFrame > >* keys) const;

public:

    QPoint _oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    ZoomContext zoomCtx;
    EventState _state;
    QMenu* _rightClickMenu;
    QColor _clearColor;
    QColor _selectedCurveColor;
    QColor _nextCurveAddedColor;
    TextRenderer _textRenderer;
    QFont* _font;
    Curves _curves;
    SelectedKeys _selectedKeyFrames;
    bool _hasOpenGLVAOSupport;
    bool _mustSetDragOrientation;
    QPoint _mouseDragOrientation; ///used to drag a key frame in only 1 direction (horizontal or vertical)
                                  ///the value is either (1,0) or (0,1)
    std::vector< KeyFrame > _keyFramesClipBoard;
    QRectF _selectionRectangle;
    QPointF _dragStartPoint;
    bool _drawSelectedKeyFramesBbox;
    QRectF _selectedKeyFramesBbox;
    QLineF _selectedKeyFramesCrossVertLine;
    QLineF _selectedKeyFramesCrossHorizLine;
    boost::shared_ptr<TimeLine> _timeline;
    bool _timelineEnabled;
    std::pair<CurveGui::SelectedDerivative,KeyPtr> _selectedDerivative;
    bool _evaluateOnPenUp; //< true if we must re-evaluate the nodes associated to the selected keyframes on penup
    QPointF _keyDragLastMovement;
    boost::scoped_ptr<QUndoStack> _undoStack;
    Gui* _gui;

    GLuint savedTexture;
private:

    QColor _baseAxisColor;
    QColor _scaleColor;
    MaxMovement _keyDragMaxMovement;
    QPolygonF _timelineTopPoly;
    QPolygonF _timelineBtmPoly;
    CurveWidget* _widget;
};

CurveWidgetPrivate::CurveWidgetPrivate(Gui* gui,
                                       boost::shared_ptr<TimeLine> timeline,
                                       CurveWidget* widget)
    : _oldClick()
      , zoomCtx()
      , _state(NONE)
      , _rightClickMenu( new QMenu(widget) )
      , _clearColor(0,0,0,255)
      , _selectedCurveColor(255,255,89,255)
      , _nextCurveAddedColor()
      , _textRenderer()
      , _font( new QFont(NATRON_FONT, NATRON_FONT_SIZE_10) )
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
      , _baseAxisColor(118,215,90,255)
      , _scaleColor(67,123,52,255)
      , _keyDragMaxMovement()
      , _timelineTopPoly()
      , _timelineBtmPoly()
      , _widget(widget)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _nextCurveAddedColor.setHsv(200,255,255);
    _rightClickMenu->setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
    _gui->registerNewUndoStack( _undoStack.get() );
}

CurveWidgetPrivate::~CurveWidgetPrivate()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    delete _font;
    for (std::list<CurveGui*>::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        delete (*it);
    }
    _curves.clear();
}

void
CurveWidgetPrivate::createMenu()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _rightClickMenu->clear();

    QMenu* fileMenu = new QMenu(_rightClickMenu);
    fileMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    fileMenu->setTitle( QObject::tr("File") );
    _rightClickMenu->addAction( fileMenu->menuAction() );

    QMenu* editMenu = new QMenu(_rightClickMenu);
    editMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    editMenu->setTitle( QObject::tr("Edit") );
    _rightClickMenu->addAction( editMenu->menuAction() );

    QMenu* interpMenu = new QMenu(_rightClickMenu);
    interpMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    interpMenu->setTitle( QObject::tr("Interpolation") );
    _rightClickMenu->addAction( interpMenu->menuAction() );

    QMenu* viewMenu = new QMenu(_rightClickMenu);
    viewMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    viewMenu->setTitle( QObject::tr("View") );
    _rightClickMenu->addAction( viewMenu->menuAction() );


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
    QPointF btmCursorLeft = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() - CURSOR_WIDTH / 2, btmcursorBtmWidgetCoord.y() );
    QPointF btmCursorRight = zoomCtx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() + CURSOR_WIDTH / 2,btmcursorBtmWidgetCoord.y() );
    QPointF topCursortop( _timeline->currentFrame(),topLeft.y() );
    QPointF topcursorTopWidgetCoord = zoomCtx.toWidgetCoordinates( topCursortop.x(),topCursortop.y() );
    QPointF topCursorBtm = zoomCtx.toZoomCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + CURSOR_HEIGHT);
    QPointF topCursorLeft = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() - CURSOR_WIDTH / 2, topcursorTopWidgetCoord.y() );
    QPointF topCursorRight = zoomCtx.toZoomCoordinates( topcursorTopWidgetCoord.x() + CURSOR_WIDTH / 2,topcursorTopWidgetCoord.y() );

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

    refreshTimelinePositions();

    QPointF topLeft = zoomCtx.toZoomCoordinates(0,0);
    QPointF btmRight = zoomCtx.toZoomCoordinates(_widget->width() - 1,_widget->height() - 1);

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
        glColor4f(0.8,0.3,0.,1.);

        glBegin(GL_LINES);
        glVertex2f( _timeline->leftBound(),btmRight.y() );
        glVertex2f( _timeline->leftBound(),topLeft.y() );
        glVertex2f( _timeline->rightBound(),btmRight.y() );
        glVertex2f( _timeline->rightBound(),topLeft.y() );
        glColor4f(0.95,0.58,0.,1.);
        glVertex2f( _timeline->currentFrame(),btmRight.y() );
        glVertex2f( _timeline->currentFrame(),topLeft.y() );
        glEnd();

        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT,GL_DONT_CARE);

        assert(_timelineBtmPoly.size() == 3 && _timelineTopPoly.size() == 3);

        glBegin(GL_POLYGON);
        glVertex2f( _timelineBtmPoly.at(0).x(),_timelineBtmPoly.at(0).y() );
        glVertex2f( _timelineBtmPoly.at(1).x(),_timelineBtmPoly.at(1).y() );
        glVertex2f( _timelineBtmPoly.at(2).x(),_timelineBtmPoly.at(2).y() );
        glEnd();

        glBegin(GL_POLYGON);
        glVertex2f( _timelineTopPoly.at(0).x(),_timelineTopPoly.at(0).y() );
        glVertex2f( _timelineTopPoly.at(1).x(),_timelineTopPoly.at(1).y() );
        glVertex2f( _timelineTopPoly.at(2).x(),_timelineTopPoly.at(2).y() );
        glEnd();
        
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
    glCheckError();
}

void
CurveWidgetPrivate::drawCurves()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    //now draw each curve
    std::vector<CurveGui*> visibleCurves;
    _widget->getVisibleCurves(&visibleCurves);
    int count = (int)visibleCurves.size();

    for (int i = 0; i < count; ++i) {
        visibleCurves[i]->drawCurve(i, count);
    }
}

void
CurveWidgetPrivate::drawBaseAxis()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    glColor4f( _baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), _baseAxisColor.alphaF() );
    glBegin(GL_LINES);
    glVertex2f(AXIS_MIN, 0);
    glVertex2f(AXIS_MAX, 0);
    glVertex2f(0, AXIS_MIN);
    glVertex2f(0, AXIS_MAX);
    glEnd();

    //reset back the color
    glColor4f(1., 1., 1., 1.);
}

void
CurveWidgetPrivate::drawScale()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _widget->context() );

    QPointF btmLeft = zoomCtx.toZoomCoordinates(0,_widget->height() - 1);
    QPointF topRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, 0);

    ///don't attempt to draw a scale on a widget with an invalid height
    if (_widget->height() <= 1) {
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

                glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);

                glBegin(GL_LINES);
                if (axis == 0) {
                    glVertex2f( value, btmLeft.y() ); // AXIS-SPECIFIC
                    glVertex2f( value, topRight.y() ); // AXIS-SPECIFIC
                } else {
                    glVertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                    glVertex2f(topRight.x(), value); // AXIS-SPECIFIC
                }
                glEnd();

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
                        QColor c = _scaleColor;
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
            double yCurve = (*it)->evaluate( openGL_pos.x() );
            double yWidget = zoomCtx.toWidgetCoordinates(0,yCurve).y();
            if (std::abs(pt.y() - yWidget) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) {
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

std::pair<CurveGui*,KeyFrame> CurveWidgetPrivate::isNearbyKeyFrame(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            KeyFrameSet keyFrames = (*it)->getInternalCurve()->getKeyFrames_mt_safe();
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

std::pair<CurveGui::SelectedDerivative,KeyPtr > CurveWidgetPrivate::isNearbyTangent(const QPoint & pt) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        QPointF leftTanPt = zoomCtx.toWidgetCoordinates( (*it)->leftTan.first,(*it)->leftTan.second );
        QPointF rightTanPt = zoomCtx.toWidgetCoordinates( (*it)->rightTan.first,(*it)->rightTan.second );
        if ( ( pt.x() >= (leftTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
             ( pt.x() <= (leftTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
             ( pt.y() <= (leftTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
             ( pt.y() >= (leftTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
            return std::make_pair(CurveGui::LEFT_TANGENT,*it);
        } else if ( ( pt.x() >= (rightTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                    ( pt.x() <= (rightTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                    ( pt.y() <= (rightTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) &&
                    ( pt.y() >= (rightTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) ) ) {
            return std::make_pair(CurveGui::RIGHT_TANGENT,*it);
        }
    }

    return std::make_pair( CurveGui::LEFT_TANGENT,KeyPtr() );
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
CurveWidgetPrivate::selectCurve(CurveGui* curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        (*it)->setSelected(false);
    }
    curve->setSelected(true);
}

void
CurveWidgetPrivate::keyFramesWithinRect(const QRectF & rect,
                                        std::vector< std::pair<CurveGui*,KeyFrame > >* keys) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double left = rect.topLeft().x();
    double right = rect.bottomRight().x();
    double bottom = rect.topLeft().y();
    double top = rect.bottomRight().y();
    for (Curves::const_iterator it = _curves.begin(); it != _curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            std::vector<KeyFrame> keysInsideRect;
            (*it)->getInternalCurve()->getKeyFramesWithinRect(left, bottom, right, top, &keysInsideRect);
            for (U32 i = 0; i < keysInsideRect.size(); ++i) {
                keys->push_back( std::make_pair(*it,keysInsideRect[i]) );
            }
        }
    }
}

void
CurveWidgetPrivate::moveSelectedKeyFrames(const QPointF & oldClick_opengl,
                                          const QPointF & newClick_opengl)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF dragStartPointOpenGL = zoomCtx.toZoomCoordinates( _dragStartPoint.x(),_dragStartPoint.y() );
    bool clampToIntegers = ( *_selectedKeyFrames.begin() )->curve->getInternalCurve()->areKeyFramesTimeClampedToIntegers();
    QPointF totalMovement;
    if (!clampToIntegers) {
        totalMovement.rx() = newClick_opengl.x() - oldClick_opengl.x();
        totalMovement.ry() = newClick_opengl.y() - oldClick_opengl.y();
    } else {
        totalMovement.rx() = newClick_opengl.x() - dragStartPointOpenGL.x();
        totalMovement.ry() = newClick_opengl.y() - dragStartPointOpenGL.y();
    }
    // clamp totalMovement to _keyDragMaxMovement
    totalMovement.rx() = std::min(std::max(totalMovement.x(),_keyDragMaxMovement.left),_keyDragMaxMovement.right);
    totalMovement.ry() = std::min(std::max(totalMovement.y(),_keyDragMaxMovement.bottom),_keyDragMaxMovement.top);

    /// round to the nearest integer the keyframes total motion (in X only)
    if (clampToIntegers) {
        totalMovement.rx() = std::floor(totalMovement.x() + 0.5);
    }

    if (clampToIntegers) {
        ///Only for the curve editor, parametric curves are not affected by the following
        for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
            if ( (*it)->curve->getInternalCurve()->areKeyFramesValuesClampedToBooleans() ) {
                totalMovement.ry() = std::max( 0.,std::min(std::floor(totalMovement.y() + 0.5),1.) );
                break;
            } else if ( (*it)->curve->getInternalCurve()->areKeyFramesValuesClampedToIntegers() ) {
                totalMovement.ry() = std::floor(totalMovement.y() + 0.5);
            }
        }
    }

    double dt;

    ///Parametric curve editor (the ones of the Parametric_Knob) never clamp keyframes to integer in the X direction
    ///We also want them to allow the user to move freely the keyframes around, hence the !clampToIntegers
    if ( (_mouseDragOrientation.x() != 0) || !clampToIntegers ) {
        if (!clampToIntegers) {
            dt =  totalMovement.x();
        } else {
            dt = totalMovement.x() - _keyDragLastMovement.x();
        }
    } else {
        dt = 0;
    }
    double dv;
    ///Parametric curve editor (the ones of the Parametric_Knob) never clamp keyframes to integer in the X direction
    ///We also want them to allow the user to move freely the keyframes around, hence the !clampToIntegers
    if ( (_mouseDragOrientation.y() != 0) || !clampToIntegers ) {
        if (!clampToIntegers) {
            dv = totalMovement.y();
        } else {
            dv = totalMovement.y() - _keyDragLastMovement.y();
        }
    } else {
        dv = 0;
    }

    if ( (dt != 0) || (dv != 0) ) {
        for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
            if ( !(*it)->curve->getInternalCurve()->isYComponentMovable() ) {
                dv = 0;
            }
        }

        if ( (dt != 0) || (dv != 0) ) {
            bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
            pushUndoCommand( new MoveKeysCommand(_widget,_selectedKeyFrames,dt,dv,!updateOnPenUpOnly) );
            _evaluateOnPenUp = true;
        }
    }
    //update last drag movement
    if ( (_mouseDragOrientation.x() != 0) || !clampToIntegers ) {
        _keyDragLastMovement.rx() = totalMovement.x();
    }
    if (_mouseDragOrientation.y() != 0) {
        _keyDragLastMovement.ry() = totalMovement.y();
    }
} // moveSelectedKeyFrames

void
CurveWidgetPrivate::moveSelectedTangent(const QPointF & pos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // the key should exist
    assert( std::find(_selectedKeyFrames.begin(),_selectedKeyFrames.end(),_selectedDerivative.second) != _selectedKeyFrames.end() );

    KeyPtr & key = _selectedDerivative.second;
    KeyFrameSet keys = key->curve->getInternalCurve()->getKeyFrames_mt_safe();
    KeyFrameSet::const_iterator cur = keys.find(key->key);
    assert( cur != keys.end() );
    double dy = key->key.getValue() - pos.y();
    double dx = key->key.getTime() - pos.x();

    //find next and previous keyframes
    KeyFrameSet::const_iterator prev = cur;
    if ( cur != keys.begin() ) {
        --prev;
    } else {
        prev = keys.end();
    }
    KeyFrameSet::const_iterator next = cur;
    ++next;

    // handle first and last keyframe correctly:
    // - if their interpolation was KEYFRAME_CATMULL_ROM or KEYFRAME_CUBIC, then it becomes KEYFRAME_FREE
    // - in all other cases it becomes KEYFRAME_BROKEN
    KeyframeType interp = key->key.getInterpolation();
    bool keyframeIsFirstOrLast = ( prev == keys.end() || next == keys.end() );
    bool interpIsNotBroken = (interp != KEYFRAME_BROKEN);
    bool interpIsCatmullRomOrCubicOrFree = (interp == KEYFRAME_CATMULL_ROM ||
                                            interp == KEYFRAME_CUBIC ||
                                            interp == KEYFRAME_FREE);
    bool setBothDerivative = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree : interpIsNotBroken;
    int keyframeIndexInCurve = key->curve->getInternalCurve()->keyFrameIndex( key->key.getTime() );

    ///If the knob is evaluating on change and _updateOnPenUpOnly is true, set it to false prior
    ///to modifying the keyframe, and set it back to its original value afterwards.
    boost::shared_ptr<KnobI> attachedKnob = key->curve->getKnob()->getKnob();
    assert(attachedKnob);
    bool isAttachedKnobEvaluatingOnChange = attachedKnob->getEvaluateOnChange();
    bool updateOnPenUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
    if (updateOnPenUpOnly && isAttachedKnobEvaluatingOnChange) {
        attachedKnob->setEvaluateOnChange(false);
    }

    // For other keyframes:
    // - if they KEYFRAME_BROKEN, move only one derivative
    // - else change to KEYFRAME_FREE and move both derivatives
    if (setBothDerivative) {
        key->key = key->curve->getInternalCurve()->setKeyFrameInterpolation(KEYFRAME_FREE, keyframeIndexInCurve);

        //if dx is not of the good sign it would make the curve uncontrollable
        if (_selectedDerivative.first == CurveGui::LEFT_TANGENT) {
            if (dx < 0) {
                dx = 1e-8;
            }
        } else {
            if (dx > 0) {
                dx = -1e-8;
            }
        }

        double derivative = dy / dx;
        key->key = key->curve->getInternalCurve()->setKeyFrameDerivatives(derivative, derivative,keyframeIndexInCurve);
    } else {
        key->key = key->curve->getInternalCurve()->setKeyFrameInterpolation(KEYFRAME_BROKEN, keyframeIndexInCurve);
        if (_selectedDerivative.first == CurveGui::LEFT_TANGENT) {
            //if dx is not of the good sign it would make the curve uncontrollable
            if (dx < 0) {
                dx = 0.0001;
            }

            double derivative = dy / dx;
            key->key = key->curve->getInternalCurve()->setKeyFrameLeftDerivative(derivative, keyframeIndexInCurve);
        } else {
            //if dx is not of the good sign it would make the curve uncontrollable
            if (dx > 0) {
                dx = -0.0001;
            }

            double derivative = dy / dx;
            key->key = key->curve->getInternalCurve()->setKeyFrameRightDerivative(derivative,keyframeIndexInCurve);
        }
    }

    ///set back its original value
    if (updateOnPenUpOnly && isAttachedKnobEvaluatingOnChange) {
        attachedKnob->setEvaluateOnChange(isAttachedKnobEvaluatingOnChange);
        _evaluateOnPenUp = true;
    }

    refreshKeyTangents(key);

    //also refresh prev/next derivatives gui if there're selected
    if ( ( prev != keys.begin() ) || ( next != keys.end() ) ) {
        for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
            if ( (*it)->curve == key->curve ) {
                if ( ( ( prev != keys.begin() ) && ( (*it)->key.getTime() == prev->getTime() ) ) ||
                     ( ( next != keys.end() ) && ( (*it)->key.getTime() == next->getTime() ) ) ) {
                    refreshKeyTangents(*it);
                }
            }
        }
    }
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
    KeyFrameSet keyframes = key->curve->getInternalCurve()->getKeyFrames_mt_safe();
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
    ++next;
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
    std::vector< std::pair<CurveGui*,KeyFrame > > keyframesSelected;
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
CurveWidgetPrivate::pushUndoCommand(QUndoCommand* cmd)
{
    _undoStack->setActive();
    _undoStack->push(cmd);
}

void
CurveWidgetPrivate::updateSelectedKeysMaxMovement()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( _selectedKeyFrames.empty() ) {
        return;
    }

    std::map<CurveGui*,MaxMovement> curvesMaxMovements;

    //for each curve that has keyframes selected,we want to find out the max movement possible on left/right
    // that means looking at the first selected key and last selected key of each curve and determining of
    //how much they can move
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        std::map<CurveGui*,MaxMovement>::iterator foundCurveMovement = curvesMaxMovements.find( (*it)->curve );

        if ( foundCurveMovement != curvesMaxMovements.end() ) {
            //if we already computed the max movement for this curve, move on
            continue;
        } else {
            assert( (*it)->curve );
            assert( (*it)->curve->getInternalCurve() );
            KeyFrameSet ks = (*it)->curve->getInternalCurve()->getKeyFrames_mt_safe();

            //find out in this set what is the first key selected
            SelectedKeys::const_iterator leftMostSelected = _selectedKeyFrames.end();
            SelectedKeys::const_iterator rightMostSelected = _selectedKeyFrames.end();
            SelectedKeys::const_iterator bottomMostSelected = _selectedKeyFrames.end();
            SelectedKeys::const_iterator topMostSelected = _selectedKeyFrames.end();
            for (SelectedKeys::const_iterator it2 = _selectedKeyFrames.begin(); it2 != _selectedKeyFrames.end(); ++it2) {
                if ( (*it2)->curve == (*it)->curve ) {
                    if ( ( leftMostSelected != _selectedKeyFrames.end() ) &&
                         ( rightMostSelected != _selectedKeyFrames.end() ) &&
                         ( bottomMostSelected != _selectedKeyFrames.end() ) &&
                         ( topMostSelected != _selectedKeyFrames.end() ) ) {
                        if ( (*it2)->key.getTime() < (*leftMostSelected)->key.getTime() ) {
                            leftMostSelected = it2;
                        }
                        if ( (*it2)->key.getTime() > (*rightMostSelected)->key.getTime() ) {
                            rightMostSelected = it2;
                        }
                        if ( (*it2)->key.getValue() < (*bottomMostSelected)->key.getValue() ) {
                            bottomMostSelected = it2;
                        }
                        if ( (*it2)->key.getValue() > (*topMostSelected)->key.getValue() ) {
                            topMostSelected = it2;
                        }
                    } else {
                        assert( leftMostSelected == _selectedKeyFrames.end() &&
                                rightMostSelected == _selectedKeyFrames.end() &&
                                bottomMostSelected == _selectedKeyFrames.end() &&
                                topMostSelected == _selectedKeyFrames.end() );
                        leftMostSelected = it2;
                        rightMostSelected = it2;
                        bottomMostSelected = it2;
                        topMostSelected = it2;
                    }
                }
            }

            //there must be a left most and right most! but they can be the same key
            assert( leftMostSelected != _selectedKeyFrames.end() && rightMostSelected != _selectedKeyFrames.end() );

            KeyFrameSet::const_iterator leftMost = ks.find( (*leftMostSelected)->key );
            KeyFrameSet::const_iterator rightMost = ks.find( (*rightMostSelected)->key );
            KeyFrameSet::const_iterator bottomMost = ks.find( (*bottomMostSelected)->key );
            KeyFrameSet::const_iterator topMost = ks.find( (*topMostSelected)->key );

            assert( leftMost != ks.end() && rightMost != ks.end() && bottomMost != ks.end() && topMost != ks.end() );

            MaxMovement curveMaxMovement;
            double minimumTimeSpanBetween2Keys = 1.;
            std::pair<double,double> curveXRange = (*it)->curve->getInternalCurve()->getXRange();
            if ( !(*it)->curve->getInternalCurve()->areKeyFramesTimeClampedToIntegers() ) {
                minimumTimeSpanBetween2Keys = NATRON_CURVE_X_SPACING_EPSILON * std::abs(curveXRange.second - curveXRange.first) * 10; //< be safe
            }


            //now get leftMostSelected's previous key to determine the max left movement for this curve
            {
                if ( leftMost == ks.begin() ) {
                    curveMaxMovement.left = curveXRange.first - leftMost->getTime();
                } else {
                    KeyFrameSet::const_iterator prev = leftMost;
                    --prev;
                    double leftMaxMovement = std::min( std::min(-NATRON_CURVE_X_SPACING_EPSILON + minimumTimeSpanBetween2Keys,0.),
                                                       prev->getTime() + minimumTimeSpanBetween2Keys - leftMost->getTime() );
                    curveMaxMovement.left = leftMaxMovement;
                    assert(curveMaxMovement.left <= 0);
                }
            }

            //now get rightMostSelected's next key to determine the max right movement for this curve
            {
                KeyFrameSet::const_iterator next = rightMost;
                ++next;
                if ( next == ks.end() ) {
                    curveMaxMovement.right = curveXRange.second - rightMost->getTime();
                } else {
                    double rightMaxMovement = std::max( std::max(NATRON_CURVE_X_SPACING_EPSILON - minimumTimeSpanBetween2Keys,0.),
                                                        next->getTime() - minimumTimeSpanBetween2Keys - rightMost->getTime() );
                    curveMaxMovement.right = rightMaxMovement;
                    assert(curveMaxMovement.right >= 0);
                }
            }

            ///Compute the max up/down movements given the min/max of the curve
            std::pair<double,double> maxY = (*it)->curve->getInternalCurve()->getCurveYRange();
            curveMaxMovement.top = maxY.second - topMost->getValue();
            curveMaxMovement.bottom = maxY.first - bottomMost->getValue();
            curvesMaxMovements.insert( std::make_pair( (*it)->curve, curveMaxMovement ) );
        }
    }

    //last step, we need to get the minimum of all curveMaxMovements to determine what is the real
    //selected keys max movement

    _keyDragMaxMovement.left = INT_MIN;
    _keyDragMaxMovement.right = INT_MAX;
    _keyDragMaxMovement.bottom = INT_MIN;
    _keyDragMaxMovement.top = INT_MAX;

    for (std::map<CurveGui*,MaxMovement>::const_iterator it = curvesMaxMovements.begin(); it != curvesMaxMovements.end(); ++it) {
        const MaxMovement & move = it->second;
        assert(move.left <= 0 && _keyDragMaxMovement.left <= 0);
        //get the minimum for the left movement (numbers are all negatives here)
        if (move.left > _keyDragMaxMovement.left) {
            _keyDragMaxMovement.left = move.left;
        }

        assert(move.right >= 0 && _keyDragMaxMovement.right >= 0);
        //get the minimum for the right movement (numbers are all positives here)
        if (move.right < _keyDragMaxMovement.right) {
            _keyDragMaxMovement.right = move.right;
        }

        assert(move.bottom <= 0 && _keyDragMaxMovement.bottom <= 0);
        if (move.bottom > _keyDragMaxMovement.bottom) {
            _keyDragMaxMovement.bottom = move.bottom;
        }

        assert(move.top >= 0 && _keyDragMaxMovement.top >= 0);
        if (move.top < _keyDragMaxMovement.top) {
            _keyDragMaxMovement.top = move.top;
        }
    }
    assert(_keyDragMaxMovement.left <= 0 && _keyDragMaxMovement.right >= 0
           && _keyDragMaxMovement.bottom <= 0 && _keyDragMaxMovement.top >= 0);
} // updateSelectedKeysMaxMovement

void
CurveWidgetPrivate::setSelectedKeysInterpolation(Natron::KeyframeType type)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::list<KeyInterpolationChange> changes;
    for (SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        KeyInterpolationChange change( (*it)->key.getInterpolation(),type,(*it) );
        changes.push_back(change);
    }
    pushUndoCommand( new SetKeysInterpolationCommand(_widget,changes) );
}

///////////////////////////////////////////////////////////////////
// CurveWidget
//

CurveWidget::CurveWidget(Gui* gui,
                         boost::shared_ptr<TimeLine> timeline,
                         QWidget* parent,
                         const QGLWidget* shareWidget)
    : QGLWidget(parent,shareWidget)
      , _imp( new CurveWidgetPrivate(gui,timeline,this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    if (timeline) {
        QObject::connect( timeline.get(),SIGNAL( frameChanged(SequenceTime,int) ),this,SLOT( onTimeLineFrameChanged(SequenceTime,int) ) );
        QObject::connect( timeline.get(),SIGNAL( boundariesChanged(SequenceTime,SequenceTime,int) ),this,SLOT( onTimeLineBoundariesChanged(SequenceTime,SequenceTime,int) ) );
        onTimeLineFrameChanged(timeline->currentFrame(), Natron::PROJECT_LOADING);
        onTimeLineBoundariesChanged(timeline->leftBound(), timeline->rightBound(), Natron::PROJECT_LOADING);
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

    if ( !glewIsSupported("GL_ARB_vertex_array_object "  // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                          ) ) {
        _imp->_hasOpenGLVAOSupport = false;
    }
}

CurveGui*
CurveWidget::createCurve(boost::shared_ptr<Curve> curve,
                         KnobGui* knob,
                         int dimension,
                         const QString & name)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    assert(knob);
    //updateGL(); //force initializeGL to be called if it wasn't before.
    CurveGui* curveGui = new CurveGui(this,curve,knob,dimension,name,QColor(255,255,255),1);
    _imp->_curves.push_back(curveGui);
    curveGui->setColor(_imp->_nextCurveAddedColor);
    _imp->_nextCurveAddedColor.setHsv( _imp->_nextCurveAddedColor.hsvHue() + 60,
                                       _imp->_nextCurveAddedColor.hsvSaturation(),_imp->_nextCurveAddedColor.value() );

    return curveGui;
}

void
CurveWidget::removeCurve(CurveGui *curve)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<CurveGui* >::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it) == curve ) {
            //remove all its keyframes from selected keys
            SelectedKeys copy;
            for (SelectedKeys::iterator it2 = _imp->_selectedKeyFrames.begin(); it2 != _imp->_selectedKeyFrames.end(); ++it2) {
                if ( (*it2)->curve != (*it) ) {
                    copy.push_back(*it2);
                }
            }
            _imp->_selectedKeyFrames = copy;

            delete (*it);
            _imp->_curves.erase(it);
            break;
        }
    }
}

void
CurveWidget::centerOn(const std::vector<CurveGui*> & curves)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( curves.empty() ) {
        return;
    }
    
    bool doCenter = false;
    RectD ret;
    for (U32 i = 0; i < curves.size(); ++i) {
        CurveGui* c = curves[i];
        if (c->getInternalCurve()->getKeyFramesCount() == 0) {
            continue;
        }
        doCenter = true;
        double xmin = c->getInternalCurve()->getMinimumTimeCovered();
        double xmax = c->getInternalCurve()->getMaximumTimeCovered();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        KeyFrameSet keys = c->getInternalCurve()->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            double value = it2->getValue();
            if (value < ymin) {
                ymin = value;
            }
            if (value > ymax) {
                ymax = value;
            }
        }
        ret.merge(xmin,ymin,xmax,ymax);
    }
    ret.set_bottom(ret.bottom() - ret.height() / 10);
    ret.set_left(ret.left() - ret.width() / 10);
    ret.set_right(ret.right() + ret.width() / 10);
    ret.set_top(ret.top() + ret.height() / 10);
    if (doCenter) {
        centerOn( ret.left(), ret.right(), ret.bottom(), ret.top() );
    }
}

void
CurveWidget::showCurvesAndHideOthers(const std::vector<CurveGui*> & curves)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<CurveGui* >::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        std::vector<CurveGui*>::const_iterator it2 = std::find(curves.begin(), curves.end(), *it);

        if ( it2 != curves.end() ) {
            (*it)->setVisible(true);
        } else {
            (*it)->setVisible(false);
        }
    }
    onCurveChanged();
}

void
CurveWidget::onCurveChanged()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///check whether selected keyframes have changed
    SelectedKeys copy;
    ///we cannot use std::transform here because a keyframe might have disappeared from a curve
    ///hence the number of keyframes selected would decrease
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        KeyFrame kf;
        bool found = (*it)->curve->getInternalCurve()->getKeyFrameWithTime( (*it)->key.getTime(), &kf );
        if (found) {
            if ( kf.getValue() != (*it)->key.getValue() ) {
                (*it)->key = kf;
            }
            _imp->refreshKeyTangents(*it);
            copy.push_back(*it);
        }
    }
    _imp->_selectedKeyFrames = copy;
    update();
}

void
CurveWidget::getVisibleCurves(std::vector<CurveGui*>* curves) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (std::list<CurveGui* >::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
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

    _imp->zoomCtx.fit(xmin, xmax, ymin, ymax);
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

    r = _imp->_clearColor.redF();
    g = _imp->_clearColor.greenF();
    b = _imp->_clearColor.blueF();
}

void
CurveWidget::saveOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
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
CurveWidget::resizeGL(int width,
                      int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if (height == 0) {
        height = 1;
    }
    glViewport (0, 0, width, height);
    _imp->zoomCtx.setScreenSize(width, height);

    if (height == 1) {
        //don't do the following when the height of the widget is irrelevant
        return;
    }

    ///find out what are the selected curves and center on them
    std::vector<CurveGui*> curves;
    getVisibleCurves(&curves);
    if ( curves.empty() ) {
        centerOn(-10,500,-10,10);
    } else {
        centerOn(curves);
    }
}

void
CurveWidget::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    glCheckError();
    {
        GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        GLProtectMatrix m(GL_MODELVIEW);
        GLProtectMatrix p(GL_PROJECTION);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        assert( 0. < _imp->zoomCtx.factor() );
        double zoomLeft, zoomRight, zoomBottom, zoomTop;
        zoomLeft = _imp->zoomCtx.left();
        zoomRight = _imp->zoomCtx.right();
        zoomBottom = _imp->zoomCtx.bottom();
        zoomTop = _imp->zoomCtx.top();
        if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
            glClearColor( _imp->_clearColor.redF(),_imp->_clearColor.greenF(),_imp->_clearColor.blueF(),_imp->_clearColor.alphaF() );
            glClear(GL_COLOR_BUFFER_BIT);

            return;
        }
        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, -1, 1);
        glCheckError();

        glClearColor( _imp->_clearColor.redF(),_imp->_clearColor.greenF(),_imp->_clearColor.blueF(),_imp->_clearColor.alphaF() );
        glClear(GL_COLOR_BUFFER_BIT);

        _imp->drawScale();

        _imp->drawBaseAxis();

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
}

void
CurveWidget::renderText(double x,
                        double y,
                        const QString & text,
                        const QColor & color,
                        const QFont & font) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( text.isEmpty() ) {
        return;
    }
    glCheckError();
    {
        GLProtectAttrib a(GL_TRANSFORM_BIT);
        GLProtectMatrix p(GL_PROJECTION);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        double h = (double)height();
        double w = (double)width();
        /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
        glOrtho(0,w,0,h,-1,1);

        QPointF pos = toWidgetCoordinates(x, y);
        glCheckError();
        _imp->_textRenderer.renderText(pos.x(),h - pos.y(),text,color,font);
        glCheckError();
    } // GLProtectAttrib a(GL_TRANSFORM_BIT);
}

void
CurveWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    ///If the click is on a curve but not nearby a keyframe, add a keyframe
    
    
    std::pair<CurveGui*,KeyFrame > selectedKey = _imp->isNearbyKeyFrame( e->pos() );
    std::pair<CurveGui::SelectedDerivative,KeyPtr > selectedTan = _imp->isNearbyTangent( e->pos() );
    if (selectedKey.first || selectedTan.second) {
        return;
    }
    
    ///We're nearby a curve
    double xCurve,yCurve;
    Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
    if ( foundCurveNearby != _imp->_curves.end() ) {
        std::vector<KeyFrame> keys(1);
        keys[0] = KeyFrame(xCurve,yCurve);
        (*foundCurveNearby)->getKnob()->pushUndoCommand(new AddKeysCommand(this,*foundCurveNearby,keys));
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

    ////
    // right button: popup menu
    if ( buttonDownIsRight(e) ) {
        _imp->createMenu();
        _imp->_rightClickMenu->exec( mapToGlobal( e->pos() ) );
        // no need to set _imp->_oldClick
        // no need to set _imp->_dragStartPoint

        // no need to updateGL()
        e->accept();
        return;
    }
    ////
    // middle button: scroll view
    if ( buttonDownIsMiddle(e) ) {
        _imp->_state = DRAGGING_VIEW;
        _imp->_oldClick = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to updateGL()
        return;
    }

    // is the click near the multiple-keyframes selection box center?
    if ( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
        // yes, start dragging
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_KEYS;
        _imp->updateSelectedKeysMaxMovement();
        _imp->_keyDragLastMovement.rx() = 0.;
        _imp->_keyDragLastMovement.ry() = 0.;
        _imp->_dragStartPoint = e->pos();
        _imp->_oldClick = e->pos();

        // no need to updateGL()
        return;
    }
    ////
    // is the click near a keyframe manipulator?
    std::pair<CurveGui*,KeyFrame > selectedKey = _imp->isNearbyKeyFrame( e->pos() );
    if (selectedKey.first) {
        _imp->_drawSelectedKeyFramesBbox = false;
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_KEYS;
        setCursor( QCursor(Qt::CrossCursor) );

        if ( !modCASIsControl(e) ) {
            _imp->_selectedKeyFrames.clear();
        }
        KeyPtr selected ( new SelectedKey(selectedKey.first,selectedKey.second) );

        _imp->refreshKeyTangents(selected);

        //insert it into the _selectedKeyFrames
        _imp->insertSelectedKeyFrameConditionnaly(selected);

        _imp->updateSelectedKeysMaxMovement();
        _imp->_keyDragLastMovement.rx() = 0.;
        _imp->_keyDragLastMovement.ry() = 0.;
        _imp->_dragStartPoint = e->pos();
        _imp->_oldClick = e->pos();
        update(); // the keyframe changes color and the derivatives must be drawn
        return;
    }
    ////
    // is the click near a derivative manipulator?
    std::pair<CurveGui::SelectedDerivative,KeyPtr > selectedTan = _imp->isNearbyTangent( e->pos() );

    //select the derivative only if it is not a constant keyframe
    if ( selectedTan.second && (selectedTan.second->key.getInterpolation() != KEYFRAME_CONSTANT) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_TANGENT;
        _imp->_selectedDerivative = selectedTan;
        _imp->_oldClick = e->pos();
        //no need to set _imp->_dragStartPoint
        update();

        return;
    }
    ////
    // is the click near the vertical current time marker?
    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_TIMELINE;
        _imp->_oldClick = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to updateGL()
        return;
    }

    ////
    // is the click near a curve?
    Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve( e->pos() );
    if ( foundCurveNearby != _imp->_curves.end() ) {
        // yes, select it and don't start any other action, the user can then do per-curve specific actions
        // like centering on it on the viewport or pasting previously copied keyframes.
        // This is kind of the last resort action before the default behaviour (which is to draw
        // a selection rectangle), because we'd rather select a keyframe than the nearby curve
        _imp->selectCurve(*foundCurveNearby);
        update();

        return;
    }

    ////
    // default behaviour: unselect selected keyframes, if any, and start a new selection
    _imp->_drawSelectedKeyFramesBbox = false;
    if ( !modCASIsControl(e) ) {
        _imp->_selectedKeyFrames.clear();
    }
    _imp->_state = SELECTING;
    _imp->_oldClick = e->pos();
    _imp->_dragStartPoint = e->pos();
    update();
} // mousePressEvent

void
CurveWidget::mouseReleaseEvent(QMouseEvent*)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (_imp->_evaluateOnPenUp) {
        _imp->_evaluateOnPenUp = false;

        if (_imp->_state == DRAGGING_KEYS) {
            std::map<KnobHolder*,bool> toEvaluate;
            for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
                boost::shared_ptr<KnobI> knob = (*it)->curve->getKnob()->getKnob();
                assert(knob);
                KnobHolder* holder = knob->getHolder();
                assert(holder);
                std::map<KnobHolder*,bool>::iterator found = toEvaluate.find(holder);
                bool evaluateOnChange = knob->getEvaluateOnChange();
                if ( ( found != toEvaluate.end() ) && !found->second && evaluateOnChange ) {
                    found->second = true;
                } else if ( found == toEvaluate.end() ) {
                    toEvaluate.insert( std::make_pair(holder,evaluateOnChange) );
                }
            }
            for (std::map<KnobHolder*,bool>::iterator it = toEvaluate.begin(); it != toEvaluate.end(); ++it) {
                it->first->evaluate_public(NULL, it->second,Natron::USER_EDITED);
            }
        } else if (_imp->_state == DRAGGING_TANGENT) {
            boost::shared_ptr<KnobI> toEvaluate = _imp->_selectedDerivative.second->curve->getKnob()->getKnob();
            assert(toEvaluate);
            toEvaluate->getHolder()->evaluate_public(toEvaluate.get(), true,Natron::USER_EDITED);
        }
    }

    EventState prevState = _imp->_state;
    _imp->_state = NONE;
    _imp->_selectionRectangle.setBottomRight( QPointF(0,0) );
    _imp->_selectionRectangle.setTopLeft( _imp->_selectionRectangle.bottomRight() );
    if (_imp->_selectedKeyFrames.size() > 1) {
        _imp->_drawSelectedKeyFramesBbox = true;
    }
    if (prevState == SELECTING) { // should other cases be considered?
        update();
    }
}

void
CurveWidget::mouseMoveEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //setFocus();

    //set cursor depending on the situation

    //find out if there is a nearby  derivative handle
    std::pair<CurveGui::SelectedDerivative,KeyPtr > selectedTan = _imp->isNearbyTangent( e->pos() );

    //if the selected keyframes rectangle is drawn and we're nearby the cross
    if ( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
        setCursor( QCursor(Qt::SizeAllCursor) );
    } else {
        //if there's a keyframe handle nearby
        std::pair<CurveGui*,KeyFrame > selectedKey = _imp->isNearbyKeyFrame( e->pos() );

        //if there's a keyframe or derivative handle nearby set the cursor to cross
        if (selectedKey.first || selectedTan.second) {
            setCursor( QCursor(Qt::CrossCursor) );
        } else {
            //if we're nearby a timeline polygon, set cursor to horizontal displacement
            if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
                setCursor( QCursor(Qt::SizeHorCursor) );
            } else {
                //default case
                setCursor( QCursor(Qt::ArrowCursor) );
            }
        }
    }

    if (_imp->_state == NONE) {
        // nothing else to do
        return;
    }

    // after this point , only mouse dragging situations are handled
    assert(_imp->_state != NONE);

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

    QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(),e->y() );
    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->_oldClick.x(),_imp->_oldClick.y() );
    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );
    switch (_imp->_state) {
    case DRAGGING_VIEW:
        _imp->zoomCtx.translate(dx, dy);
        break;

    case DRAGGING_KEYS:
        if (!_imp->_mustSetDragOrientation) {
            if ( !_imp->_selectedKeyFrames.empty() ) {
                bool clampToIntegers = ( *_imp->_selectedKeyFrames.begin() )->curve->getInternalCurve()->areKeyFramesTimeClampedToIntegers();
                if (!clampToIntegers) {
                    _imp->updateSelectedKeysMaxMovement();
                }
                _imp->moveSelectedKeyFrames(oldClick_opengl,newClick_opengl);
            }
        }
        break;

    case SELECTING:
        _imp->refreshSelectionRectangle( (double)e->x(),(double)e->y() );
        break;

    case DRAGGING_TANGENT:
        _imp->moveSelectedTangent(newClick_opengl);
        break;

    case DRAGGING_TIMELINE:
        _imp->_timeline->seekFrame( (SequenceTime)newClick_opengl.x(),NULL, Natron::CURVE_EDITOR_SEEK );
        break;

    case NONE:
        assert(0);
        break;
    }

    _imp->_oldClick = e->pos();

    update();
} // mouseMoveEvent

void
CurveWidget::refreshSelectedKeysBbox()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    RectD keyFramesBbox;
    for (SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin();
         it != _imp->_selectedKeyFrames.end();
         ++it) {
        double x = (*it)->key.getTime();
        double y = (*it)->key.getValue();
        if ( it != _imp->_selectedKeyFrames.begin() ) {
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
            keyFramesBbox.set_left(x);
            keyFramesBbox.set_right(x);
            keyFramesBbox.set_top(y);
            keyFramesBbox.set_bottom(y);
        }
    }
    QPointF topLeft( keyFramesBbox.left(),keyFramesBbox.top() );
    QPointF btmRight( keyFramesBbox.right(),keyFramesBbox.bottom() );
    _imp->_selectedKeyFramesBbox.setTopLeft(topLeft);
    _imp->_selectedKeyFramesBbox.setBottomRight(btmRight);

    QPointF middle( ( topLeft.x() + btmRight.x() ) / 2., ( topLeft.y() + btmRight.y() ) / 2. );
    QPointF middleWidgetCoord = toWidgetCoordinates( middle.x(),middle.y() );
    QPointF middleLeft = _imp->zoomCtx.toZoomCoordinates( middleWidgetCoord.x() - 20,middleWidgetCoord.y() );
    QPointF middleRight = _imp->zoomCtx.toZoomCoordinates( middleWidgetCoord.x() + 20,middleWidgetCoord.y() );
    QPointF middleTop = _imp->zoomCtx.toZoomCoordinates(middleWidgetCoord.x(),middleWidgetCoord.y() - 20);
    QPointF middleBottom = _imp->zoomCtx.toZoomCoordinates(middleWidgetCoord.x(),middleWidgetCoord.y() + 20);

    _imp->_selectedKeyFramesCrossHorizLine.setPoints(middleLeft,middleRight);
    _imp->_selectedKeyFramesCrossVertLine.setPoints(middleBottom,middleTop);
}

void
CurveWidget::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    const double zoomFactor_min = 0.0001;
    const double zoomFactor_max = 10000.;
    const double par_min = 0.0001;
    const double par_max = 10000.;
    double zoomFactor;
    double par;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( modCASIsControlShift(e) ) {
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        par = _imp->zoomCtx.par() / scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.par();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else if ( modCASIsControl(e) ) {
        // Alt + Wheel: zoom time only, keep point under mouse
        par = _imp->zoomCtx.par() * scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.par();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else {
        // Wheel: zoom values and time, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }

    if (_imp->_drawSelectedKeyFramesBbox) {
        refreshSelectedKeysBbox();
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

QSize
CurveWidget::sizeHint() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (parentWidget()->objectName() == "CurveEditorSplitter") {
        ///if this is the curve widget associated to the CurveEditor
        return QSize(400,1000);
    } else {
        ///a random parametric param curve editor
        return QSize(400,400);
    }
}

void
CurveWidget::addKeyFrame(CurveGui* curve,
                         const KeyFrame & key)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    curve->getInternalCurve()->addKeyFrame(key);
    if ( curve->getInternalCurve()->isAnimated() ) {
        curve->setVisibleAndRefresh(true);
    }
}

#if 0 // dead code
void
CurveWidget::removeKeyFrame(CurveGui* curve,
                            const KeyFrame & key)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    curve->getInternalCurve()->removeKeyFrameWithTime( key.getTime() );
    if ( !curve->getInternalCurve()->isAnimated() ) {
        curve->setVisibleAndRefresh(false);
    }
    SelectedKeys::iterator it = _imp->_selectedKeyFrames.find( SelectedKey(curve,key) );
    if ( it != _imp->_selectedKeyFrames.end() ) {
        _imp->_selectedKeyFrames.erase(it);
    }
}

#endif

void
CurveWidget::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) {
        if ( parentWidget() ) {
            if ( parentWidget()->parentWidget() ) {
                if (parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName) {
                    QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
                    QCoreApplication::postEvent(parentWidget()->parentWidget(),ev);
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys, modifiers, key) ) {
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
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenter, modifiers, key) ) {
        frameSelectedCurve();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll, modifiers, key) ) {
        selectAllKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCopy, modifiers, key) ) {
        copySelectedKeyFrames();
    } else if ( isKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorPaste, modifiers, key) ) {
        pasteKeyFramesFromClipBoardToSelectedCurve();
    }  else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if (_imp->_timeline) {
            _imp->_timeline->goToPreviousKeyframe();
        }
    }  else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if (_imp->_timeline) {
            _imp->_timeline->goToNextKeyframe();
        }
    }
} // keyPressEvent

void
CurveWidget::enterEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QWidget* currentFocus = qApp->focusWidget();
    
    bool canSetFocus = !currentFocus ||
    dynamic_cast<ViewerGL*>(currentFocus) ||
    dynamic_cast<CurveWidget*>(currentFocus) ||
    dynamic_cast<Histogram*>(currentFocus) ||
    dynamic_cast<NodeGraph*>(currentFocus) ||
    currentFocus->objectName() == "Properties" ||
    currentFocus->objectName() == "tree";
    
    if (canSetFocus) {
        setFocus();
    }
    
    QGLWidget::enterEvent(e);
}

//struct RefreshTangent_functor{
//    CurveWidgetPrivate* _imp;
//
//    RefreshTangent_functor(CurveWidgetPrivate* imp): _imp(imp){}
//
//    SelectedKey operator()(SelectedKey key){
//        _imp->refreshKeyTangents(key);
//        return key;
//    }
//};

void
CurveWidget::refreshDisplayedTangents()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        _imp->refreshKeyTangents(*it);
    }
    update();
}

void
CurveWidget::setSelectedKeys(const SelectedKeys & keys)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_selectedKeyFrames = keys;
    refreshDisplayedTangents();
    refreshSelectedKeysBbox();
}

void
CurveWidget::refreshSelectedKeys()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    refreshDisplayedTangents();
    refreshSelectedKeysBbox();
}

void
CurveWidget::constantInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_CONSTANT);
}

void
CurveWidget::linearInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_LINEAR);
}

void
CurveWidget::smoothForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_SMOOTH);
}

void
CurveWidget::catmullromInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_CATMULL_ROM);
}

void
CurveWidget::cubicInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_CUBIC);
}

void
CurveWidget::horizontalInterpForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_HORIZONTAL);
}

void
CurveWidget::breakDerivativesForSelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->setSelectedKeysInterpolation(KEYFRAME_BROKEN);
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
    _imp->_selectedKeyFramesBbox.setBottomRight( QPointF(0,0) );
    _imp->_selectedKeyFramesBbox.setTopLeft( _imp->_selectedKeyFramesBbox.bottomRight() );

    //apply the same strategy than for moveSelectedKeyFrames()

    std::map<KnobGui*,std::vector< std::pair<CurveGui*,KeyFrame > > > toRemove;
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        std::map<KnobGui*,std::vector< std::pair<CurveGui*,KeyFrame > > >::iterator foundKnob = toRemove.find( (*it)->curve->getKnob() );

        if ( foundKnob == toRemove.end() ) {
            std::vector< std::pair<CurveGui*,KeyFrame > > newV;
            newV.push_back( std::make_pair( (*it)->curve, (*it)->key ) );
            toRemove.insert( std::make_pair( (*it)->curve->getKnob(), newV ) );
        } else {
            foundKnob->second.push_back( std::make_pair( (*it)->curve, (*it)->key ) );
        }
    }

    for (std::map<KnobGui*,std::vector< std::pair<CurveGui*,KeyFrame > > >::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        it->first->pushUndoCommand( new RemoveKeysCommand(this,it->second) );
    }

    _imp->_selectedKeyFrames.clear();

    update();
}

void
CurveWidget::copySelectedKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_keyFramesClipBoard.clear();
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end(); ++it) {
        _imp->_keyFramesClipBoard.push_back( (*it)->key );
    }
}

void
CurveWidget::pasteKeyFramesFromClipBoardToSelectedCurve()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    CurveGui* curve = NULL;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isSelected() ) {
            curve = (*it);
            break;
        }
    }
    if (!curve) {
        warningDialog( QObject::tr("Curve Editor").toStdString(),QObject::tr("You must select a curve first.").toStdString() );

        return;
    }
    //this function will call updateGL() for us
    curve->getKnob()->pushUndoCommand( new AddKeysCommand(this,curve, _imp->_keyFramesClipBoard) );
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
CurveWidget::selectAllKeyFrames()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->_drawSelectedKeyFramesBbox = true;
    _imp->_selectedKeyFrames.clear();
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            KeyFrameSet keys = (*it)->getInternalCurve()->getKeyFrames_mt_safe();
            for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                KeyPtr newSelected( new SelectedKey(*it,*it2) );
                _imp->refreshKeyTangents(newSelected);
                _imp->insertSelectedKeyFrameConditionnaly(newSelected);
            }
        }
    }
    refreshSelectedKeysBbox();
    update();
}

void
CurveWidget::frameSelectedCurve()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isSelected() ) {
            std::vector<CurveGui*> curves;
            curves.push_back(*it);
            centerOn(curves);

            return;
        }
    }
    warningDialog( tr("Curve Editor").toStdString(),tr("You must select a curve first.").toStdString() );
}

void
CurveWidget::onTimeLineFrameChanged(SequenceTime,
                                    int /*reason*/)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    if (_imp->_gui->isGUIFrozen()) {
        return;
    }

    if (!_imp->_timelineEnabled) {
        _imp->_timelineEnabled = true;
    }
    _imp->refreshTimelinePositions();
    update();
}

bool
CurveWidget::isTabVisible() const
{
    if ( parentWidget() ) {
        QWidget* parent  = parentWidget()->parentWidget();
        if (parent) {
            if (parent->objectName() == kCurveEditorObjectName) {
                TabWidget* tab = dynamic_cast<TabWidget*>( parentWidget()->parentWidget()->parentWidget() );
                if (tab) {
                    if ( tab->isFloatingWindowChild() ) {
                        return true;
                    }

                    return tab->currentWidget() == parent;
                }
            }
        }
    }

    return false;
}

void
CurveWidget::onTimeLineBoundariesChanged(SequenceTime,
                                         SequenceTime,
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

bool
CurveWidget::isSupportingOpenGLVAO() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->_hasOpenGLVAOSupport;
}

const QFont &
CurveWidget::getTextFont() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return *_imp->_font;
}

void
CurveWidget::getProjection(double *zoomLeft,
                           double *zoomBottom,
                           double *zoomFactor,
                           double *zoomPAR) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomPAR = _imp->zoomCtx.par();
}

void
CurveWidget::setProjection(double zoomLeft,
                           double zoomBottom,
                           double zoomFactor,
                           double zoomPAR)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomPAR);
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

    _imp->_undoStack->setActive();
}

void
CurveWidget::exportCurveToAscii()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<CurveGui*> curves;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            curves.push_back(*it);
        }
    }
    if ( curves.empty() ) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must have a curve on the editor first.").toStdString() );

        return;
    }

    ImportExportCurveDialog dialog(true,curves,_imp->_gui,this);
    if ( dialog.exec() ) {
        double x = dialog.getXStart();
        double end = dialog.getXEnd();
        double incr = dialog.getXIncrement();
        std::map<int,CurveGui*> columns;
        dialog.getCurveColumns(&columns);

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            double endInt = std::floor(end);
            if ( curves[i]->getInternalCurve()->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) || ( endInt != end) ) ) {
                warningDialog( tr("Curve Export").toStdString(),curves[i]->getName().toStdString() + tr(" doesn't support X values that are not integers.").toStdString() );

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

        for (double i = x; i <= end; i += incr) {
            for (int c = 0; c < columnsCount; ++c) {
                std::map<int,CurveGui*>::const_iterator foundCurve = columns.find(c);
                if ( foundCurve != columns.end() ) {
                    QString str = QString::number(foundCurve->second->evaluate(i),'f',10);
                    ts << str;
                } else {
                    ts <<  0;
                }
                if (c < columnsCount - 1) {
                    ts << '_';
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

    std::vector<CurveGui*> curves;
    for (Curves::iterator it = _imp->_curves.begin(); it != _imp->_curves.end(); ++it) {
        if ( (*it)->isVisible() ) {
            curves.push_back(*it);
        }
    }
    if ( curves.empty() ) {
        warningDialog( tr("Curve Editor").toStdString(),tr("You must have a curve on the editor first.").toStdString() );

        return;
    }

    ImportExportCurveDialog dialog(false,curves,_imp->_gui,this);
    if ( dialog.exec() ) {
        QString filePath = dialog.getFilePath();
        if ( !QFile::exists(filePath) ) {
            warningDialog( tr("Curve Import").toStdString(),tr("File not found.").toStdString() );

            return;
        }

        double x = dialog.getXStart();
        double incr = dialog.getXIncrement();
        std::map<int,CurveGui*> columns;
        dialog.getCurveColumns(&columns);
        assert( !columns.empty() );

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            if ( curves[i]->getInternalCurve()->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) ) ) {
                warningDialog( tr("Curve Import").toStdString(),curves[i]->getName().toStdString() + tr(" doesn't support X values that are not integers.").toStdString() );

                return;
            }
        }

        QFile file( dialog.getFilePath() );
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        std::map<CurveGui*, std::vector<double> > curvesValues;
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
                while ( i < line.size() && line.at(i) != QChar('_') ) {
                    value.push_back( line.at(i) );
                    ++i;
                }
                if ( i < line.size() ) {
                    if ( line.at(i) != QChar('_') ) {
                        errorDialog( tr("Curve Import").toStdString(),tr("The file could not be read.").toStdString() );

                        return;
                    }
                    ++i;
                }
                bool ok;
                double v = value.toDouble(&ok);
                if (!ok) {
                    errorDialog( tr("Curve Import").toStdString(),tr("The file could not be read.").toStdString() );

                    return;
                }
                values.push_back(v);
            }
            ///assert that the values count is greater than the number of curves provided by the user
            if ( values.size() < columns.size() ) {
                errorDialog( tr("Curve Import").toStdString(),tr("The file contains less curves than what you selected.").toStdString() );

                return;
            }

            for (std::map<int,CurveGui*>::const_iterator col = columns.begin(); col != columns.end(); ++col) {
                if ( col->first >= (int)values.size() ) {
                    errorDialog( tr("Curve Import").toStdString(),tr("One of the curve column index is not a valid index for the given file.").toStdString() );

                    return;
                }
                std::map<CurveGui*, std::vector<double> >::iterator foundCurve = curvesValues.find(col->second);
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
        for (std::map<CurveGui*, std::vector<double> >::const_iterator it = curvesValues.begin(); it != curvesValues.end(); ++it) {
            const std::vector<double> & values = it->second;
            CurveGui* curve = it->first;
            curve->getInternalCurve()->clearKeyFrames();

            double xIndex = x;
            for (U32 i = 0; i < values.size(); ++i) {
                KeyFrame k(xIndex,values[i]);
                curve->getInternalCurve()->addKeyFrame(k);
                xIndex += incr;
            }
        }
        _imp->_selectedKeyFrames.clear();
        update();
    }
} // importCurveFromAscii

ImportExportCurveDialog::ImportExportCurveDialog(bool isExportDialog,
                                                 const std::vector<CurveGui*> & curves,
                                                 Gui* gui,
                                                 QWidget* parent)
    : QDialog(parent)
      , _gui(gui)
      , _isExportDialog(isExportDialog)
      , _mainLayout(0)
      , _fileContainer(0)
      , _fileLayout(0)
      , _fileLabel(0)
      , _fileLineEdit(0)
      , _fileBrowseButton(0)
      , _startContainer(0)
      , _startLayout(0)
      , _startLabel(0)
      , _startSpinBox(0)
      , _incrContainer(0)
      , _incrLayout(0)
      , _incrLabel(0)
      , _incrSpinBox(0)
      , _endContainer(0)
      , _endLayout(0)
      , _endLabel(0)
      , _endSpinBox(0)
      , _curveColumns()
      , _buttonsContainer(0)
      , _buttonsLayout(0)
      , _okButton(0)
      , _cancelButton(0)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 3, 0, 0);
    _mainLayout->setSpacing(2);
    setLayout(_mainLayout);

    //////File
    _fileContainer = new QWidget(this);
    _fileLayout = new QHBoxLayout(_fileContainer);
    _fileLabel = new QLabel(tr("File:"),_fileContainer);
    _fileLayout->addWidget(_fileLabel);
    _fileLineEdit = new LineEdit(_fileContainer);
    _fileLineEdit->setPlaceholderText( tr("File path...") );
    _fileLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _fileLayout->addWidget(_fileLineEdit);
    _fileBrowseButton = new Button(_fileContainer);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _fileBrowseButton->setIcon( QIcon(pix) );
    QObject::connect( _fileBrowseButton, SIGNAL( clicked() ), this, SLOT( open_file() ) );
    _fileLayout->addWidget(_fileBrowseButton);
    _mainLayout->addWidget(_fileContainer);

    //////x start value
    _startContainer = new QWidget(this);
    _startLayout = new QHBoxLayout(_startContainer);
    _startLabel = new QLabel(tr("X start value:"),_startContainer);
    _startLayout->addWidget(_startLabel);
    _startSpinBox = new SpinBox(_startContainer,SpinBox::DOUBLE_SPINBOX);
    _startSpinBox->setValue(0);
    _startLayout->addWidget(_startSpinBox);
    _mainLayout->addWidget(_startContainer);

    //////x increment
    _incrContainer = new QWidget(this);
    _incrLayout = new QHBoxLayout(_incrContainer);
    _incrLabel = new QLabel(tr("X increment:"),_incrContainer);
    _incrLayout->addWidget(_incrLabel);
    _incrSpinBox = new SpinBox(_incrContainer,SpinBox::DOUBLE_SPINBOX);
    _incrSpinBox->setValue(0.01);
    _incrLayout->addWidget(_incrSpinBox);
    _mainLayout->addWidget(_incrContainer);

    //////x end value
    if (isExportDialog) {
        _endContainer = new QWidget(this);
        _endLayout = new QHBoxLayout(_endContainer);
        _endLabel = new QLabel(tr("X end value:"),_endContainer);
        _endLayout->addWidget(_endLabel);
        _endSpinBox = new SpinBox(_endContainer,SpinBox::DOUBLE_SPINBOX);
        _endSpinBox->setValue(1);
        _endLayout->addWidget(_endSpinBox);
        _mainLayout->addWidget(_endContainer);
    }

    ////curves columns
    double min = 0,max = 0;
    bool curveIsClampedToIntegers = false;
    for (U32 i = 0; i < curves.size(); ++i) {
        CurveColumn column;
        double curvemin = curves[i]->getInternalCurve()->getMinimumTimeCovered();
        double curvemax = curves[i]->getInternalCurve()->getMaximumTimeCovered();
        if (curvemin < min) {
            min = curvemin;
        }
        if (curvemax > max) {
            max = curvemax;
        }
        if ( curves[i]->getInternalCurve()->areKeyFramesTimeClampedToIntegers() ) {
            curveIsClampedToIntegers = true;
        }
        column._curve = curves[i];
        column._curveContainer = new QWidget(this);
        column._curveLayout = new QHBoxLayout(column._curveContainer);
        column._curveLabel = new QLabel( curves[i]->getName() + tr(" column:") );
        column._curveLayout->addWidget(column._curveLabel);
        column._curveSpinBox = new SpinBox(column._curveContainer,SpinBox::INT_SPINBOX);
        column._curveSpinBox->setValue( (double)i + 1. );
        column._curveLayout->addWidget(column._curveSpinBox);
        _curveColumns.push_back(column);
        _mainLayout->addWidget(column._curveContainer);
    }
    if (isExportDialog) {
        _startSpinBox->setValue(min);
        _endSpinBox->setValue(max);
    }
    if (curveIsClampedToIntegers) {
        _incrSpinBox->setValue(1);
    }
    /////buttons
    _buttonsContainer = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsContainer);
    _okButton = new Button(tr("Ok"),_buttonsContainer);
    QObject::connect( _okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    _buttonsLayout->addWidget(_okButton);
    _cancelButton = new Button(tr("Cancel"),_buttonsContainer);
    QObject::connect( _cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
    _buttonsLayout->addWidget(_cancelButton);
    _mainLayout->addWidget(_buttonsContainer);
}

void
ImportExportCurveDialog::open_file()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<std::string> filters;
    filters.push_back("*");
    if (_isExportDialog) {
        SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::SAVE_DIALOG,"",_gui,false);
        if ( dialog.exec() ) {
            std::string file = dialog.filesToSave();
            _fileLineEdit->setText( file.c_str() );
        }
    } else {
        SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::OPEN_DIALOG,"",_gui,false);
        if ( dialog.exec() ) {
            std::string files = dialog.selectedFiles();
            if ( !files.empty() ) {
                _fileLineEdit->setText( files.c_str() );
            }
        }
    }
}

QString
ImportExportCurveDialog::getFilePath()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _fileLineEdit->text();
}

double
ImportExportCurveDialog::getXStart() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _startSpinBox->value();
}

double
ImportExportCurveDialog::getXIncrement() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _incrSpinBox->value();
}

double
ImportExportCurveDialog::getXEnd() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///only valid for export dialogs
    assert(_isExportDialog);

    return _endSpinBox->value();
}

void
ImportExportCurveDialog::getCurveColumns(std::map<int,CurveGui*>* columns) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (U32 i = 0; i < _curveColumns.size(); ++i) {
        columns->insert( std::make_pair( (int)(_curveColumns[i]._curveSpinBox->value() - 1),_curveColumns[i]._curve ) );
    }
}


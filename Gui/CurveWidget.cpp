//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "CurveWidget.h"

#include <QMenu>
#include <QMouseEvent>
#include <QtCore/QCoreApplication>
#include <QtCore/QRectF>
#include <QtGui/QPolygonF>

#include "Global/AppManager.h"
#include "Engine/Knob.h"
#include "Engine/Rect.h"
#include "Engine/TimeLine.h"
#include "Engine/Variant.h"
#include "Engine/Curve.h"

#include "Gui/ScaleSlider.h"
#include "Gui/ticks.h"
#include "Gui/CurveEditor.h"
#include "Gui/TextRenderer.h"

using namespace Natron;

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
// (in widget pixels)
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

static double AXIS_MAX = 100000.;
static double AXIS_MIN = -100000.;


namespace { // protect local classes in anonymous namespace

enum EventState {
    DRAGGING_VIEW = 0,
    DRAGGING_KEYS = 1,
    SELECTING = 2,
    DRAGGING_TANGENT = 3,
    DRAGGING_TIMELINE = 4,
    NONE = 5
};

}

struct SelectedKey {
    CurveGui* curve;
    boost::shared_ptr<KeyFrame> key;
    std::pair<double,double> leftTan, rightTan;
};

CurveGui::CurveGui(const CurveWidget *curveWidget,
                   boost::shared_ptr<Curve> curve,
                   const QString& name,
                   const QColor& color,
                   int thickness)
    : _internalCurve(curve)
    , _name(name)
    , _color(color)
    , _thickness(thickness)
    , _visible(false)
    , _selected(false)
    , _curveWidget(curveWidget)
{
    
    if(curve->keyFramesCount() > 1){
        _visible = true;
    }
    
    QObject::connect(this,SIGNAL(curveChanged()),curveWidget,SLOT(updateGL()));
    
}

CurveGui::~CurveGui(){
    
}

boost::shared_ptr<KeyFrame> CurveGui::nextPointForSegment(double x1, double* x2){
    const KeyFrameSet& keys = _internalCurve->getKeyFrames();
    assert(!keys.empty());
    double xminCurveWidgetCoord = _curveWidget->toWidgetCoordinates((*keys.begin())->getTime(),0).x();
    double xmaxCurveWidgetCoord = _curveWidget->toWidgetCoordinates((*keys.rbegin())->getTime(),0).x();
    if(x1 < xminCurveWidgetCoord){
        *x2 = xminCurveWidgetCoord;
    }else if(x1 >= xmaxCurveWidgetCoord){
        *x2 = _curveWidget->width() - 1;
    }else {
        //we're between 2 keyframes,get the upper and lower
        KeyFrameSet::const_iterator upper = keys.end();
        double upperWidgetCoord = x1;
        for(KeyFrameSet::const_iterator it = keys.begin();it!=keys.end();++it){
            upperWidgetCoord = _curveWidget->toWidgetCoordinates((*it)->getTime(),0).x();
            if(upperWidgetCoord > x1){
                upper = it;
                break;
            }
        }
        assert(upper != keys.end() && upper!= keys.begin());
        
        KeyFrameSet::const_iterator lower = upper;
        --lower;
        
        double t = ( x1 - (*lower)->getTime() ) / ((*upper)->getTime() - (*lower)->getTime());
        double P3 = (*upper)->getValue().value<double>();
        double P0 = (*lower)->getValue().value<double>();
        double P3pl = (*upper)->getLeftTangent().value<double>();
        double P0pr = (*lower)->getRightTangent().value<double>();
        double secondDer = 6. * (1. - t) *(P3 - P3pl / 3. - P0 - 2. * P0pr / 3.) +
                6.* t * (P0 - P3 + 2 * P3pl / 3. + P0pr / 3. );
        double secondDerWidgetCoord = std::abs(_curveWidget->toWidgetCoordinates(0,secondDer).y()
                                               / ((*upper)->getTime() - (*lower)->getTime()));
        // compute delta_x so that the y difference between the tangent and the curve is at most
        // 1 pixel (use the second order Taylor expansion of the function)
        double delta_x = std::max(2. / std::max(std::sqrt(secondDerWidgetCoord),0.1), 1.);
        
        if(upperWidgetCoord < x1 + delta_x){
            *x2 = upperWidgetCoord;
            return (*upper);
        }else{
            *x2 = x1 + delta_x;
        }
    }
    return boost::shared_ptr<KeyFrame>();
    
}

void CurveGui::drawCurve(){
    if(!_visible)
        return;
    
    assert(QGLContext::currentContext() == _curveWidget->context());
    
    std::vector<float> vertices;
    double x1 = 0;
    double x2;
    double w = _curveWidget->width();
    boost::shared_ptr<KeyFrame> isX1AKey;
    while(x1 < (w -1)){
        double x,y;
        if(!isX1AKey){
            x = _curveWidget->toScaleCoordinates(x1,0).x();
            y = evaluate(x);
        }else{
            x = isX1AKey->getTime();
            y = isX1AKey->getValue().value<double>();
        }
        vertices.push_back((float)x);
        vertices.push_back((float)y);
        isX1AKey = nextPointForSegment(x1,&x2);
        x1 = x2;
    }
    //also add the last point
    {
        double x = _curveWidget->toScaleCoordinates(x1,0).x();
        double y = evaluate(x);
        vertices.push_back((float)x);
        vertices.push_back((float)y);
    }
    
    const QColor& curveColor = _selected ?  _curveWidget->getSelectedCurveColor() : _color;
    
    glColor4f(curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF());
    
    glPointSize(_thickness);
    glPushAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glLineWidth(1.5);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    glBegin(GL_LINE_STRIP);
    for(int i = 0; i < (int)vertices.size();i+=2){
        glVertex2f(vertices[i],vertices[i+1]);
    }
    glEnd();
    
    
    
    
    
    //render the name of the curve
    glColor4f(1.f, 1.f, 1.f, 1.f);
    double textX = _curveWidget->toScaleCoordinates(15,0).x();
    double textY = evaluate(textX);
    _curveWidget->renderText(textX,textY,_name,_color,_curveWidget->getFont());
    glColor4f(curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF());
    
    
    //draw keyframes
    const KeyFrameSet& keyframes = _internalCurve->getKeyFrames();
    glPointSize(7.f);
    glEnable(GL_POINT_SMOOTH);
    
    const SelectedKeys& selectedKeyFrames = _curveWidget->getSelectedKeyFrames();
    for(KeyFrameSet::const_iterator k = keyframes.begin();k!=keyframes.end();++k){
        glColor4f(_color.redF(), _color.greenF(), _color.blueF(), _color.alphaF());
        boost::shared_ptr<KeyFrame> key = (*k);
        //if the key is selected change its color to white
        SelectedKeys::const_iterator isSelected = selectedKeyFrames.end();
        for(SelectedKeys::const_iterator it2 = selectedKeyFrames.begin();
            it2 != selectedKeyFrames.end();++it2){
            if((*it2)->key == key){
                isSelected = it2;
                glColor4f(1.f,1.f,1.f,1.f);
                break;
            }
        }
        double x = key->getTime();
        double y = key->getValue().toDouble();
        glBegin(GL_POINTS);
        glVertex2f(x,y);
        glEnd();
        if(isSelected != selectedKeyFrames.end() && key->getInterpolation() != KEYFRAME_CONSTANT){

            //draw the tangents lines
            if(key->getInterpolation() != KEYFRAME_FREE && key->getInterpolation() != KEYFRAME_BROKEN){
                glLineStipple(2, 0xAAAA);
                glEnable(GL_LINE_STIPPLE);
            }
            glBegin(GL_LINES);
            glColor4f(1., 0.35, 0.35, 1.);
            glVertex2f((*isSelected)->leftTan.first, (*isSelected)->leftTan.second);
            glVertex2f(x, y);
            glVertex2f(x, y);
            glVertex2f((*isSelected)->rightTan.first, (*isSelected)->rightTan.second);
            glEnd();
            if(key->getInterpolation() != KEYFRAME_FREE && key->getInterpolation() != KEYFRAME_BROKEN){
                glDisable(GL_LINE_STIPPLE);
            }
            
            if(selectedKeyFrames.size() == 1){ //if one keyframe, also draw the coordinates
                QString coordStr("x: %1, y: %2");
                coordStr = coordStr.arg(x).arg(y);
                double yWidgetCoord = _curveWidget->toWidgetCoordinates(0,key->getValue().toDouble()).y();
                QFontMetrics m(_curveWidget->getFont());
                yWidgetCoord += (m.height() + 4);
                glColor4f(1., 1., 1., 1.);
                _curveWidget->renderText(x, _curveWidget->toScaleCoordinates(0, yWidgetCoord).y(),
                                         coordStr, QColor(240,240,240), _curveWidget->getFont());
                
            }
            glBegin(GL_POINTS);
            glVertex2f((*isSelected)->leftTan.first, (*isSelected)->leftTan.second);
            glVertex2f((*isSelected)->rightTan.first, (*isSelected)->rightTan.second);
            glEnd();
        }
        
    }
    
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.);
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
    glPopAttrib();
    glPointSize(1.f);
    //reset back the color
    glColor4f(1.f, 1.f, 1.f, 1.f);
    checkGLErrors();

    
}

double CurveGui::evaluate(double x) const{
    return _internalCurve->getValueAt(x).toDouble();
}

void CurveGui::setVisible(bool visible){ _visible = visible; }

void CurveGui::setVisibleAndRefresh(bool visible) { _visible = visible; emit curveChanged(); }


/*****************************CURVE WIDGET***********************************************/

namespace { // protext local classes in anonymous namespace

// a data container with only a constructor, a destructor, and a few utility functions is really just a struct
// see ViewerGL.cpp for a full documentation of ZoomContext
struct ZoomContext {

    ZoomContext()
        : bottom(0.)
        , left(0.)
        , zoomFactor(1.)
        , aspectRatio(0.1)
    {}

    QPoint _oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    double bottom; /// the bottom edge of orthographic projection
    double left; /// the left edge of the orthographic projection
    double zoomFactor; /// the zoom factor applied to the current image
    double aspectRatio;///

    double _lastOrthoLeft,_lastOrthoBottom,_lastOrthoRight,_lastOrthoTop; //< remembers the last values passed to the glOrtho call
};

typedef std::list<CurveGui* > Curves;

}

// although all members are public, CurveWidgetPrivate is really a class because it has lots of member functions
// (in fact, many members could probably be made private)
class CurveWidgetPrivate {
public:

    CurveWidgetPrivate(boost::shared_ptr<TimeLine> timeline,CurveWidget* widget);

    ~CurveWidgetPrivate();

    void drawSelectionRectangle();

    void refreshTimelinePositions();

    void drawTimelineMarkers();

    void drawCurves();

    void drawBaseAxis();

    void drawScale();
    void drawSelectedKeyFramesBbox();

    Curves::const_iterator isNearbyCurve(const QPoint &pt) const;


    std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > isNearbyKeyFrame(const QPoint& pt) const;

    std::pair<CurveGui::SelectedTangent,SelectedKeys::const_iterator > isNearByTangent(const QPoint& pt) const;

    bool isNearbySelectedKeyFramesCrossWidget(const QPoint& pt) const;

    bool isNearbyTimelineTopPoly(const QPoint& pt) const;

    bool isNearbyTimelineBtmPoly(const QPoint& pt) const;

    /**
     * @brief Selects the curve given in parameter and deselects any other curve in the widget.
     **/
    void selectCurve(CurveGui* curve);

    void moveSelectedKeyFrames(const QPointF& oldClick_opengl,const QPointF& newClick_opengl);

    void moveSelectedTangent(const QPointF& pos);
    
    void refreshKeyTangentsGUI(boost::shared_ptr<SelectedKey> key);

    void refreshSelectionRectangle(double x,double y);

    void updateSelectedKeysMaxMovement();
    
    void setKeysInterpolation(const std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >& keys,Natron::KeyframeType type);

private:

    void createMenu();

    void keyFramesWithinRect(const QRectF& rect,std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >* keys) const;

public:

    ZoomContext _zoomCtx;
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

    std::vector< std::pair<double,Variant> > _keyFramesClipBoard;
    QRectF _selectionRectangle;
    QPointF _dragStartPoint;
    bool _drawSelectedKeyFramesBbox;
    QRectF _selectedKeyFramesBbox;
    QLineF _selectedKeyFramesCrossVertLine;
    QLineF _selectedKeyFramesCrossHorizLine;
    boost::shared_ptr<TimeLine> _timeline;
    bool _timelineEnabled;
    std::pair<CurveGui::SelectedTangent,SelectedKeys::const_iterator> _selectedTangent;
    
private:

    QColor _baseAxisColor;
    QColor _scaleColor;
    QPointF _keyDragMaxMovement;
    QPolygonF _timelineTopPoly;
    QPolygonF _timelineBtmPoly;
    CurveWidget* _widget;

};

CurveWidgetPrivate::CurveWidgetPrivate(boost::shared_ptr<TimeLine> timeline,CurveWidget* widget)
    : _zoomCtx()
    , _state(NONE)
    , _rightClickMenu(new QMenu(widget))
    , _clearColor(0,0,0,255)
    , _selectedCurveColor(255,255,89,255)
    , _nextCurveAddedColor()
    , _textRenderer()
    , _font(new QFont(NATRON_FONT, NATRON_FONT_SIZE_10))
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
    , _selectedTangent()
    , _baseAxisColor(118,215,90,255)
    , _scaleColor(67,123,52,255)
    , _keyDragMaxMovement()
    , _timelineTopPoly()
    , _timelineBtmPoly()
    , _widget(widget)
{
    _nextCurveAddedColor.setHsv(200,255,255);
    createMenu();
}

CurveWidgetPrivate::~CurveWidgetPrivate() {
    delete _font;
    for (std::list<CurveGui*>::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        delete (*it);
    }
    _curves.clear();
}

void CurveWidgetPrivate::createMenu() {
    _rightClickMenu->clear();
    QMenu* editMenu = new QMenu(_rightClickMenu);
    editMenu->setTitle("Edit");
    _rightClickMenu->addAction(editMenu->menuAction());

    QMenu* interpMenu = new QMenu(_rightClickMenu);
    interpMenu->setTitle("Interpolation");
    _rightClickMenu->addAction(interpMenu->menuAction());

    QMenu* viewMenu = new QMenu(_rightClickMenu);
    viewMenu->setTitle("View");
    _rightClickMenu->addAction(viewMenu->menuAction());

    QAction* deleteKeyFramesAction = new QAction("Delete selected keyframes",_widget);
    deleteKeyFramesAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    QObject::connect(deleteKeyFramesAction,SIGNAL(triggered()),_widget,SLOT(deleteSelectedKeyFrames()));
    editMenu->addAction(deleteKeyFramesAction);

    QAction* copyKeyFramesAction = new QAction("Copy selected keyframes",_widget);
    copyKeyFramesAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    QObject::connect(copyKeyFramesAction,SIGNAL(triggered()),_widget,SLOT(copySelectedKeyFrames()));
    editMenu->addAction(copyKeyFramesAction);

    QAction* pasteKeyFramesAction = new QAction("Paste to selected curve",_widget);
    pasteKeyFramesAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    QObject::connect(pasteKeyFramesAction,SIGNAL(triggered()),_widget,SLOT(pasteKeyFramesFromClipBoardToSelectedCurve()));
    editMenu->addAction(pasteKeyFramesAction);

    QAction* selectAllAction = new QAction("Select all keyframes",_widget);
    selectAllAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    QObject::connect(selectAllAction,SIGNAL(triggered()),_widget,SLOT(selectAllKeyFrames()));
    editMenu->addAction(selectAllAction);


    QAction* constantInterp = new QAction("Constant",_widget);
    constantInterp->setShortcut(QKeySequence(Qt::Key_K));
    QObject::connect(constantInterp,SIGNAL(triggered()),_widget,SLOT(constantInterpForSelectedKeyFrames()));
    interpMenu->addAction(constantInterp);

    QAction* linearInterp = new QAction("Linear",_widget);
    linearInterp->setShortcut(QKeySequence(Qt::Key_L));
    QObject::connect(linearInterp,SIGNAL(triggered()),_widget,SLOT(linearInterpForSelectedKeyFrames()));
    interpMenu->addAction(linearInterp);


    QAction* smoothInterp = new QAction("Smooth",_widget);
    smoothInterp->setShortcut(QKeySequence(Qt::Key_Z));
    QObject::connect(smoothInterp,SIGNAL(triggered()),_widget,SLOT(smoothForSelectedKeyFrames()));
    interpMenu->addAction(smoothInterp);


    QAction* catmullRomInterp = new QAction("Catmull-Rom",_widget);
    catmullRomInterp->setShortcut(QKeySequence(Qt::Key_R));
    QObject::connect(catmullRomInterp,SIGNAL(triggered()),_widget,SLOT(catmullromInterpForSelectedKeyFrames()));
    interpMenu->addAction(catmullRomInterp);


    QAction* cubicInterp = new QAction("Cubic",_widget);
    cubicInterp->setShortcut(QKeySequence(Qt::Key_C));
    QObject::connect(cubicInterp,SIGNAL(triggered()),_widget,SLOT(cubicInterpForSelectedKeyFrames()));
    interpMenu->addAction(cubicInterp);

    QAction* horizontalInterp = new QAction("Horizontal",_widget);
    horizontalInterp->setShortcut(QKeySequence(Qt::Key_H));
    QObject::connect(horizontalInterp,SIGNAL(triggered()),_widget,SLOT(horizontalInterpForSelectedKeyFrames()));
    interpMenu->addAction(horizontalInterp);


    QAction* breakTangents = new QAction("Break",_widget);
    breakTangents->setShortcut(QKeySequence(Qt::Key_X));
    QObject::connect(breakTangents,SIGNAL(triggered()),_widget,SLOT(breakTangentsForSelectedKeyFrames()));
    interpMenu->addAction(breakTangents);

    QAction* frameCurve = new QAction("Frame selected curve",_widget);
    frameCurve->setShortcut(QKeySequence(Qt::Key_F));
    QObject::connect(frameCurve,SIGNAL(triggered()),_widget,SLOT(frameSelectedCurve()));
    viewMenu->addAction(frameCurve);
}

void CurveWidgetPrivate::drawSelectionRectangle() {
    glPushAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);

    glColor4f(0.3,0.3,0.3,0.2);
    QPointF btmRight = _selectionRectangle.bottomRight();
    QPointF topLeft = _selectionRectangle.topLeft();

    glBegin(GL_POLYGON);
    glVertex2f(topLeft.x(),btmRight.y());
    glVertex2f(topLeft.x(),topLeft.y());
    glVertex2f(btmRight.x(),topLeft.y());
    glVertex2f(btmRight.x(),btmRight.y());
    glEnd();


    glLineWidth(1.5);

    glColor4f(0.5,0.5,0.5,1.);
    glBegin(GL_LINE_STRIP);
    glVertex2f(topLeft.x(),btmRight.y());
    glVertex2f(topLeft.x(),topLeft.y());
    glVertex2f(btmRight.x(),topLeft.y());
    glVertex2f(btmRight.x(),btmRight.y());
    glVertex2f(topLeft.x(),btmRight.y());
    glEnd();


    glDisable(GL_LINE_SMOOTH);
    checkGLErrors();

    glLineWidth(1.);
    glPopAttrib();
    glColor4f(1., 1., 1., 1.);
}

void CurveWidgetPrivate::refreshTimelinePositions() {
    QPointF topLeft = _widget->toScaleCoordinates(0,0);
    QPointF btmRight = _widget->toScaleCoordinates(_widget->width()-1,_widget->height()-1);

    QPointF btmCursorBtm(_timeline->currentFrame(),btmRight.y());
    QPointF btmcursorBtmWidgetCoord = _widget->toWidgetCoordinates(btmCursorBtm.x(),btmCursorBtm.y());
    QPointF btmCursorTop = _widget->toScaleCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - CURSOR_HEIGHT);
    QPointF btmCursorLeft = _widget->toScaleCoordinates(btmcursorBtmWidgetCoord.x() - CURSOR_WIDTH /2, btmcursorBtmWidgetCoord.y());
    QPointF btmCursorRight = _widget->toScaleCoordinates(btmcursorBtmWidgetCoord.x() + CURSOR_WIDTH / 2,btmcursorBtmWidgetCoord.y());

    QPointF topCursortop(_timeline->currentFrame(),topLeft.y());
    QPointF topcursorTopWidgetCoord = _widget->toWidgetCoordinates(topCursortop.x(),topCursortop.y());
    QPointF topCursorBtm = _widget->toScaleCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + CURSOR_HEIGHT);
    QPointF topCursorLeft = _widget->toScaleCoordinates(topcursorTopWidgetCoord.x() - CURSOR_WIDTH /2, topcursorTopWidgetCoord.y());
    QPointF topCursorRight = _widget->toScaleCoordinates(topcursorTopWidgetCoord.x() + CURSOR_WIDTH / 2,topcursorTopWidgetCoord.y());

    _timelineBtmPoly.clear();
    _timelineTopPoly.clear();

    _timelineBtmPoly.push_back(btmCursorTop);
    _timelineBtmPoly.push_back(btmCursorLeft);
    _timelineBtmPoly.push_back(btmCursorRight);

    _timelineTopPoly.push_back(topCursorBtm);
    _timelineTopPoly.push_back(topCursorLeft);
    _timelineTopPoly.push_back(topCursorRight);
}

void CurveWidgetPrivate::drawTimelineMarkers() {
    refreshTimelinePositions();

    QPointF topLeft = _widget->toScaleCoordinates(0,0);
    QPointF btmRight = _widget->toScaleCoordinates(_widget->width()-1,_widget->height()-1);

    glPushAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glColor4f(0.8,0.3,0.,1.);

    glBegin(GL_LINES);
    glVertex2f(_timeline->leftBound(),btmRight.y());
    glVertex2f(_timeline->leftBound(),topLeft.y());
    glVertex2f(_timeline->rightBound(),btmRight.y());
    glVertex2f(_timeline->rightBound(),topLeft.y());
    glColor4f(0.95,0.58,0.,1.);
    glVertex2f(_timeline->currentFrame(),btmRight.y());
    glVertex2f(_timeline->currentFrame(),topLeft.y());
    glEnd();

    glDisable(GL_LINE_SMOOTH);


    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_DONT_CARE);

    assert(_timelineBtmPoly.size() == 3 && _timelineTopPoly.size() == 3);

    glBegin(GL_POLYGON);
    glVertex2f(_timelineBtmPoly.at(0).x(),_timelineBtmPoly.at(0).y());
    glVertex2f(_timelineBtmPoly.at(1).x(),_timelineBtmPoly.at(1).y());
    glVertex2f(_timelineBtmPoly.at(2).x(),_timelineBtmPoly.at(2).y());
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(_timelineTopPoly.at(0).x(),_timelineTopPoly.at(0).y());
    glVertex2f(_timelineTopPoly.at(1).x(),_timelineTopPoly.at(1).y());
    glVertex2f(_timelineTopPoly.at(2).x(),_timelineTopPoly.at(2).y());
    glEnd();

    glDisable(GL_POLYGON_SMOOTH);
    glPopAttrib();
}

void CurveWidgetPrivate::drawCurves()
{
    assert(QGLContext::currentContext() == _widget->context());
    //now draw each curve
    for (std::list<CurveGui*>::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        (*it)->drawCurve();
    }
}

void CurveWidgetPrivate::drawBaseAxis()
{
    assert(QGLContext::currentContext() == _widget->context());

    glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), _baseAxisColor.alphaF());
    glBegin(GL_LINES);
    glVertex2f(AXIS_MIN, 0);
    glVertex2f(AXIS_MAX, 0);
    glVertex2f(0, AXIS_MIN);
    glVertex2f(0, AXIS_MAX);
    glEnd();

    //reset back the color
    glColor4f(1., 1., 1., 1.);
}

void CurveWidgetPrivate::drawScale()
{
    assert(QGLContext::currentContext() == _widget->context());

    QPointF btmLeft = _widget->toScaleCoordinates(0,_widget->height()-1);
    QPointF topRight = _widget->toScaleCoordinates(_widget->width()-1, 0);


    QFontMetrics fontM(*_font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);

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
        const double minTickSizeTextPixel = (axis == 0) ? fontM.width(QString("00")) : fontM.height(); // AXIS-SPECIFIC
        const double minTickSizeText = range * minTickSizeTextPixel/rangePixel;
        for (int i = m1 ; i <= m2; ++i) {
            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i-m1]*smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

            glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);

            glBegin(GL_LINES);
            if (axis == 0) {
                glVertex2f(value, btmLeft.y()); // AXIS-SPECIFIC
                glVertex2f(value, topRight.y()); // AXIS-SPECIFIC
            } else {
                glVertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                glVertex2f(topRight.x(), value); // AXIS-SPECIFIC
            }
            glEnd();

            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize/range;
                const QString s = QString::number(value);
                const int sSizePixel = (axis == 0) ? fontM.width(s) : fontM.height(); // AXIS-SPECIFIC
                if (tickSizePixel > sSizePixel) {
                    const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                    double alphaText = 1.0;//alpha;
                    if (tickSizePixel < sSizeFullPixel) {
                        // when the text size is between sSizePixel and sSizeFullPixel,
                        // draw it with a lower alpha
                        alphaText *= (tickSizePixel - sSizePixel)/(double)minTickSizeTextPixel;
                    }
                    QColor c = _scaleColor;
                    c.setAlpha(255*alphaText);
                    if (axis == 0) {
                        _widget->renderText(value, btmLeft.y(), s, c, *_font); // AXIS-SPECIFIC
                    } else {
                        _widget->renderText(btmLeft.x(), value, s, c, *_font); // AXIS-SPECIFIC
                    }
                }
            }
        }

    }

    glDisable(GL_BLEND);
    //reset back the color
    glColor4f(1., 1., 1., 1.);

}

void CurveWidgetPrivate::drawSelectedKeyFramesBbox() {

    glPushAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);


    QPointF topLeft = _selectedKeyFramesBbox.topLeft();
    QPointF btmRight = _selectedKeyFramesBbox.bottomRight();

    glLineWidth(1.5);

    glColor4f(0.5,0.5,0.5,1.);
    glBegin(GL_LINE_STRIP);
    glVertex2f(topLeft.x(),btmRight.y());
    glVertex2f(topLeft.x(),topLeft.y());
    glVertex2f(btmRight.x(),topLeft.y());
    glVertex2f(btmRight.x(),btmRight.y());
    glVertex2f(topLeft.x(),btmRight.y());
    glEnd();


    glBegin(GL_LINES);
    glVertex2f(std::max(_selectedKeyFramesCrossHorizLine.p1().x(),topLeft.x()),_selectedKeyFramesCrossHorizLine.p1().y());
    glVertex2f(std::min(_selectedKeyFramesCrossHorizLine.p2().x(),btmRight.x()),_selectedKeyFramesCrossHorizLine.p2().y());
    glVertex2f(_selectedKeyFramesCrossVertLine.p1().x(),std::max(_selectedKeyFramesCrossVertLine.p1().y(),btmRight.y()));
    glVertex2f(_selectedKeyFramesCrossVertLine.p2().x(),std::min(_selectedKeyFramesCrossVertLine.p2().y(),topLeft.y()));
    glEnd();

    glDisable(GL_LINE_SMOOTH);
    checkGLErrors();

    glLineWidth(1.);
    glPopAttrib();
    glColor4f(1., 1., 1., 1.);
}


Curves::const_iterator CurveWidgetPrivate::isNearbyCurve(const QPoint &pt) const {
    QPointF openGL_pos = _widget->toScaleCoordinates(pt.x(),pt.y());
    for (Curves::const_iterator it = _curves.begin();it!=_curves.end();++it) {
        if ((*it)->isVisible()) {
            double y = (*it)->evaluate(openGL_pos.x());
            double yWidget = _widget->toWidgetCoordinates(0,y).y();
            if (std::abs(pt.y() - yWidget) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) {
                return it;
            }
        }
    }
    return _curves.end();
}


std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > CurveWidgetPrivate::isNearbyKeyFrame(const QPoint& pt) const {
    for (Curves::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        if ((*it)->isVisible()) {
            const KeyFrameSet& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (KeyFrameSet::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                QPointF keyFramewidgetPos = _widget->toWidgetCoordinates((*it2)->getTime(), (*it2)->getValue().toDouble());
                if ((std::abs(pt.y() - keyFramewidgetPos.y()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                        (std::abs(pt.x() - keyFramewidgetPos.x()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
                    return std::make_pair((*it),(*it2));
                }
            }
        }
    }
    return std::make_pair((CurveGui*)NULL,boost::shared_ptr<KeyFrame>());
}

std::pair<CurveGui::SelectedTangent,SelectedKeys::const_iterator > CurveWidgetPrivate::isNearByTangent(const QPoint& pt) const {
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it!=_selectedKeyFrames.end(); ++it) {
        QPointF leftTanPt = _widget->toWidgetCoordinates((*it)->leftTan.first,(*it)->leftTan.second);
        QPointF rightTanPt = _widget->toWidgetCoordinates((*it)->rightTan.first,(*it)->rightTan.second);
        if (pt.x() >= (leftTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                pt.x() <= (leftTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                pt.y() <= (leftTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                pt.y() >= (leftTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
            return std::make_pair(CurveGui::LEFT_TANGENT,it);
        } else if (pt.x() >= (rightTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   pt.x() <= (rightTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   pt.y() <= (rightTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   pt.y() >= (rightTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
            return std::make_pair(CurveGui::RIGHT_TANGENT,it);
        }

    }
    return std::make_pair(CurveGui::LEFT_TANGENT,_selectedKeyFrames.end());
}

bool CurveWidgetPrivate::isNearbySelectedKeyFramesCrossWidget(const QPoint& pt) const {


    QPointF middleLeft = _widget->toWidgetCoordinates(_selectedKeyFramesCrossHorizLine.p1().x(),_selectedKeyFramesCrossHorizLine.p1().y());
    QPointF middleRight = _widget->toWidgetCoordinates(_selectedKeyFramesCrossHorizLine.p2().x(),_selectedKeyFramesCrossHorizLine.p2().y());
    QPointF middleBtm = _widget->toWidgetCoordinates(_selectedKeyFramesCrossVertLine.p1().x(),_selectedKeyFramesCrossVertLine.p1().y());
    QPointF middleTop = _widget->toWidgetCoordinates(_selectedKeyFramesCrossVertLine.p2().x(),_selectedKeyFramesCrossVertLine.p2().y());

    if (pt.x() >= (middleLeft.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
            pt.x() <= (middleRight.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
            pt.y() <= (middleLeft.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
            pt.y() >= (middleLeft.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
        //is nearby horizontal line
        return true;
    } else if(pt.y() >= (middleBtm.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
              pt.y() <= (middleTop.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
              pt.x() <= (middleBtm.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
              pt.x() >= (middleBtm.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
        //is nearby vertical line
        return true;
    } else {
        return false;
    }

}


bool CurveWidgetPrivate::isNearbyTimelineTopPoly(const QPoint& pt) const {
    QPointF pt_opengl = _widget->toScaleCoordinates(pt.x(),pt.y());
    return _timelineTopPoly.containsPoint(pt_opengl,Qt::OddEvenFill);
}

bool CurveWidgetPrivate::isNearbyTimelineBtmPoly(const QPoint& pt) const {
    QPointF pt_opengl = _widget->toScaleCoordinates(pt.x(),pt.y());
    return _timelineBtmPoly.containsPoint(pt_opengl,Qt::OddEvenFill);
}

/**
 * @brief Selects the curve given in parameter and deselects any other curve in the widget.
 **/
void CurveWidgetPrivate::selectCurve(CurveGui* curve) {
    for (Curves::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        (*it)->setSelected(false);
    }
    curve->setSelected(true);
}


void CurveWidgetPrivate::keyFramesWithinRect(const QRectF& rect,std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >* keys) const {

    double left = rect.topLeft().x();
    double right = rect.bottomRight().x();
    double bottom = rect.topLeft().y();
    double top = rect.bottomRight().y();
    for (Curves::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        const KeyFrameSet& keyframes = (*it)->getInternalCurve()->getKeyFrames();
        for (KeyFrameSet::const_iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
            double y = (*it2)->getValue().value<double>();
            double x = (*it2)->getTime() ;
            if (x <= right && x >= left && y <= top && y >= bottom) {
                keys->push_back(std::make_pair(*it,*it2));
            }
        }
    }
}

void CurveWidgetPrivate::moveSelectedKeyFrames(const QPointF& oldClick_opengl,const QPointF& newClick_opengl) {
    QPointF dragStartPointOpenGL = _widget->toScaleCoordinates(_dragStartPoint.x(),_dragStartPoint.y());
    QPointF translation = (newClick_opengl - oldClick_opengl);
    translation.rx() *= _mouseDragOrientation.x();
    translation.ry() *= _mouseDragOrientation.y();

    CurveEditor* editor = NULL;
    if (_widget->parentWidget()) {
        if (_widget->parentWidget()->parentWidget()) {
            if (_widget->parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName) {
                editor = dynamic_cast<CurveEditor*>(_widget->parentWidget()->parentWidget());
            }
        }
    }
    std::vector<std::pair< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> >, std::pair<double, Variant> > > moves;
    //1st off, round to the nearest integer the keyframes total motion

    double totalMovement = std::floor(newClick_opengl.x() - dragStartPointOpenGL.x() + 0.5);
    // clamp totalMovement to _keyDragMaxMovement
    if (totalMovement < 0) {
        totalMovement = std::max(totalMovement,_keyDragMaxMovement.x());
    } else {
        totalMovement = std::min(totalMovement,_keyDragMaxMovement.y());
    }

    double lastMovement = std::floor(oldClick_opengl.x() - dragStartPointOpenGL.x() + 0.5);
    if (lastMovement < 0) {
        lastMovement = std::max(lastMovement,_keyDragMaxMovement.x());
    } else {
        lastMovement = std::min(lastMovement,_keyDragMaxMovement.y());
    }
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
        double newX;
        if (_mouseDragOrientation.x() != 0) {
            newX = (*it)->key->getTime() + totalMovement - lastMovement;
        } else {
            newX = (*it)->key->getTime();
        }
        double newY = (*it)->key->getValue().toDouble() + translation.y();

        if (!editor) {
            (*it)->curve->getInternalCurve()->setKeyFrameValueAndTime(newX,Variant(newY), (*it)->key);
        } else {
            if (_selectedKeyFrames.size() > 1) {
                moves.push_back(std::make_pair(std::make_pair((*it)->curve,(*it)->key),std::make_pair(newX,Variant(newY))));
            } else {
                editor->setKeyFrame((*it)->curve,(*it)->key,newX,Variant(newY));
            }
        }
    }
    if (editor && _selectedKeyFrames.size() > 1) {
        editor->setKeyFrames(moves);
        //the editor redo() call will call refreshSelectedKeysBbox() for us
        //and also call refreshDisplayedTangents()
    } else {
        _widget->refreshSelectedKeysBbox();
        //refresh now the tangents positions
        _widget->refreshDisplayedTangents();
    }


}

void CurveWidgetPrivate::moveSelectedTangent(const QPointF& pos) {

    assert(_selectedTangent.second != _selectedKeyFrames.end());

    boost::shared_ptr<SelectedKey> key = (*_selectedTangent.second);
    const KeyFrameSet& keys = key->curve->getInternalCurve()->getKeyFrames();

    KeyFrameSet::const_iterator cur = std::find(keys.begin(),keys.end(),key->key);
    assert(cur != keys.end());
    double dy = key->key->getValue().toDouble() - pos.y();
    double dx = key->key->getTime() - pos.x();

    //find next and previous keyframes
    KeyFrameSet::const_iterator prev = cur;
    if (cur != keys.begin()) {
        --prev;
    } else {
        prev = keys.end();
    }
    KeyFrameSet::const_iterator next = cur;
    ++next;

    // handle first and last keyframe correctly:
    // - if their interpolation was KEYFRAME_CATMULL_ROM or KEYFRAME_CUBIC, then it becomes KEYFRAME_FREE
    // - in all other cases it becomes KEYFRAME_BROKEN
    KeyframeType interp = key->key->getInterpolation();
    bool keyframeIsFirstOrLast = (prev == keys.end() || next == keys.end());
    bool interpIsNotBroken = (interp != KEYFRAME_BROKEN);
    bool interpIsCatmullRomOrCubicOrFree = (interp == KEYFRAME_CATMULL_ROM ||
                                            interp == KEYFRAME_CUBIC ||
                                            interp == KEYFRAME_FREE);
    bool setBothTangent = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree : interpIsNotBroken;


    // For other keyframes:
    // - if they KEYFRAME_BROKEN, move only one tangent
    // - else change to KEYFRAME_FREE and move both tangents
    if (setBothTangent) {
        key->key->setInterpolation(KEYFRAME_FREE);

        //if dx is not of the good sign it would make the curve uncontrollable
        if (_selectedTangent.first == CurveGui::LEFT_TANGENT) {
            if (dx < 0) {
                dx = 1e-8;
            }
        } else {
            if (dx > 0) {
                dx = -1e-8;
            }
        }

        double leftTan,rightTan;
        if (prev != keys.end()) {
            leftTan = (dy * ((*cur)->getTime() - (*prev)->getTime())) / dx;
        } else {
            leftTan = dy / dx;
        }
        if (next != keys.end()) {
            rightTan = (dy * ((*next)->getTime() - (*cur)->getTime())) / dx;
        } else {
            rightTan = dy / dx;
        }
        key->curve->getInternalCurve()->setKeyFrameTangents(Variant(leftTan),Variant(rightTan),key->key);

    } else {
        key->key->setInterpolation(KEYFRAME_BROKEN);
        if (_selectedTangent.first == CurveGui::LEFT_TANGENT) {
            //if dx is not of the good sign it would make the curve uncontrollable
            if (dx < 0) {
                dx = 0.0001;
            }

            double leftTan;
            if (prev != keys.end()) {
                leftTan = (dy * ((*cur)->getTime() - (*prev)->getTime())) / dx;
            } else {
                leftTan = dy / dx;
            }
            key->curve->getInternalCurve()->setKeyFrameLeftTangent(Variant(leftTan),key->key);

        } else {
            //if dx is not of the good sign it would make the curve uncontrollable
            if (dx > 0) {
                dx = -0.0001;
            }

            double rightTan;
            if (next != keys.end()) {
                rightTan = (dy * ((*next)->getTime() - (*cur)->getTime())) / dx;
            } else {
                rightTan = dy / dx;
            }
            key->curve->getInternalCurve()->setKeyFrameRightTangent(Variant(rightTan),key->key);
        }
    }
    refreshKeyTangentsGUI(key);


}

void CurveWidgetPrivate::refreshKeyTangentsGUI(boost::shared_ptr<SelectedKey> key) {
    double w = (double)_widget->width();
    double h = (double)_widget->height();
    double x = key->key->getTime();
    double y = key->key->getValue().toDouble();
    QPointF keyWidgetCoord = _widget->toWidgetCoordinates(x,y);
    const KeyFrameSet& keyframes = key->curve->getInternalCurve()->getKeyFrames();
    KeyFrameSet::const_iterator k = std::find(keyframes.begin(),keyframes.end(),key->key);
    assert(k != keyframes.end());

    //find the previous and next keyframes on the curve to find out the  position of the tangents
    KeyFrameSet::const_iterator prev = k;
    if (k != keyframes.begin()) {
        --prev;
    } else {
        prev = keyframes.end();
    }
    KeyFrameSet::const_iterator next = k;
    ++next;
    double leftTanX, leftTanY;
    {
        double prevTime = (prev == keyframes.end()) ? (x - 1.) : (*prev)->getTime();
        double leftTan = key->key->getLeftTangent().toDouble()/(x - prevTime);
        double leftTanXWidgetDiffMax = w / 8.;
        if (prev != keyframes.end()) {
            double prevKeyXWidgetCoord = _widget->toWidgetCoordinates(prevTime, 0).x();
            //set the left tangent X to be at 1/3 of the interval [prev,k], and clamp it to 1/8 of the widget width.
            leftTanXWidgetDiffMax = std::min(leftTanXWidgetDiffMax, (keyWidgetCoord.x() - prevKeyXWidgetCoord) / 3.);
        }
        //clamp the left tangent Y to 1/8 of the widget height.
        double leftTanYWidgetDiffMax = std::min( h/8., leftTanXWidgetDiffMax);
        assert(leftTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(leftTanYWidgetDiffMax >= 0.);

        QPointF tanMax = _widget->toScaleCoordinates(keyWidgetCoord.x() + leftTanXWidgetDiffMax, keyWidgetCoord.y() -leftTanYWidgetDiffMax) - QPointF(x,y);
        assert(tanMax.x() >= 0.); // both should be positive
        assert(tanMax.y() >= 0.);

        if (tanMax.x() * std::abs(leftTan) < tanMax.y()) {
            leftTanX = x - tanMax.x();
            leftTanY = y - tanMax.x() * leftTan;
        } else {
            leftTanX = x - tanMax.y() / std::abs(leftTan);
            leftTanY = y - tanMax.y() * (leftTan > 0 ? 1 : -1);
        }
        assert(std::abs(leftTanX - x) <= tanMax.x()*1.001); // check that they are effectively clamped (taking into account rounding errors)
        assert(std::abs(leftTanY - y) <= tanMax.y()*1.001);
    }
    double rightTanX, rightTanY;
    {
        double nextTime = (next == keyframes.end()) ? (x + 1.) : (*next)->getTime();
        double rightTan = key->key->getRightTangent().toDouble()/(nextTime - x );
        double rightTanXWidgetDiffMax = w / 8.;
        if (next != keyframes.end()) {
            double nextKeyXWidgetCoord = _widget->toWidgetCoordinates(nextTime, 0).x();
            //set the right tangent X to be at 1/3 of the interval [k,next], and clamp it to 1/8 of the widget width.
            rightTanXWidgetDiffMax = std::min(rightTanXWidgetDiffMax, (nextKeyXWidgetCoord - keyWidgetCoord.x()) / 3.);
        }
        //clamp the right tangent Y to 1/8 of the widget height.
        double rightTanYWidgetDiffMax = std::min( h/8., rightTanXWidgetDiffMax);
        assert(rightTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(rightTanYWidgetDiffMax >= 0.);

        QPointF tanMax = _widget->toScaleCoordinates(keyWidgetCoord.x() + rightTanXWidgetDiffMax, keyWidgetCoord.y() -rightTanYWidgetDiffMax) - QPointF(x,y);
        assert(tanMax.x() >= 0.); // both bounds should be positive
        assert(tanMax.y() >= 0.);

        if (tanMax.x() * std::abs(rightTan) < tanMax.y()) {
            rightTanX = x + tanMax.x();
            rightTanY = y + tanMax.x() * rightTan;
        } else {
            rightTanX = x + tanMax.y() / std::abs(rightTan);
            rightTanY = y + tanMax.y() * (rightTan > 0 ? 1 : -1);
        }
        assert(std::abs(rightTanX - x) <= tanMax.x()*1.001); // check that they are effectively clamped (taking into account rounding errors)
        assert(std::abs(rightTanY - y) <= tanMax.y()*1.001);
    }
    key->leftTan.first = leftTanX;
    key->leftTan.second = leftTanY;
    key->rightTan.first = rightTanX;
    key->rightTan.second = rightTanY;

}

void CurveWidgetPrivate::refreshSelectionRectangle(double x,double y) {
    double xmin = std::min(_dragStartPoint.x(),x);
    double xmax = std::max(_dragStartPoint.x(),x);
    double ymin = std::min(_dragStartPoint.y(),y);
    double ymax = std::max(_dragStartPoint.y(),y);
    _selectionRectangle.setBottomRight(_widget->toScaleCoordinates(xmax,ymin));
    _selectionRectangle.setTopLeft(_widget->toScaleCoordinates(xmin,ymax));
    std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > > keyframesSelected;
    keyFramesWithinRect(_selectionRectangle,&keyframesSelected);
    for (U32 i = 0; i < keyframesSelected.size(); ++i) {
        std::pair<SelectedKeys::const_iterator,bool> found = _widget->isKeySelected(keyframesSelected[i].second);
        if (!found.second) {
            boost::shared_ptr<SelectedKey> newSelected(new SelectedKey);
            newSelected->key = keyframesSelected[i].second;
            newSelected->curve = keyframesSelected[i].first;
            refreshKeyTangentsGUI(newSelected);
            _selectedKeyFrames.push_back(newSelected);
        }
    }
    _widget->refreshSelectedKeysBbox();
}

void CurveWidgetPrivate::updateSelectedKeysMaxMovement() {
    if (_selectedKeyFrames.empty()) {
        return;
    }

    //find out the min/max of the selection
    boost::shared_ptr<SelectedKey> leftMost,rightMost;
    for(SelectedKeys::const_iterator it = _selectedKeyFrames.begin();it!=_selectedKeyFrames.end();++it){
        if (!leftMost) {
            leftMost = *it;
        } else {
            if ((*it)->key->getTime() < leftMost->key->getTime()) {
                leftMost = *it;
            }
        }
        if (!rightMost) {
            rightMost = *it;
        } else {
            if ((*it)->key->getTime() > rightMost->key->getTime()) {
                rightMost = *it;
            }
        }

    }

    //find out for leftMost and rightMost of how much they can move respectively on the left and on the right
    {
        const KeyFrameSet& leftMostCurveKeys = leftMost->curve->getInternalCurve()->getKeyFrames();
        KeyFrameSet::const_iterator foundLeft = std::find(leftMostCurveKeys.begin(),
                                                        leftMostCurveKeys.end(),leftMost->key);
        assert(foundLeft != leftMostCurveKeys.end());
        if (foundLeft == leftMostCurveKeys.begin()) {
            _keyDragMaxMovement.setX(INT_MIN);
        } else {
            KeyFrameSet::const_iterator foundLeftPrev = foundLeft;
            --foundLeftPrev;
            _keyDragMaxMovement.setX(((*foundLeftPrev)->getTime() + 1) - leftMost->key->getTime());
        }
    }

    {
        const KeyFrameSet& rightMostCurveKeys = rightMost->curve->getInternalCurve()->getKeyFrames();
        KeyFrameSet::const_iterator foundRight = std::find(rightMostCurveKeys.begin(),
                                                         rightMostCurveKeys.end(),rightMost->key);
        assert(foundRight != rightMostCurveKeys.end());
        KeyFrameSet::const_iterator foundRightNext = foundRight;
        ++foundRightNext;

        if (foundRightNext == rightMostCurveKeys.end()) {
            _keyDragMaxMovement.setY(INT_MAX);
        } else {
            _keyDragMaxMovement.setY((*foundRightNext)->getTime() - 1 - rightMost->key->getTime());
        }
    }


}

void CurveWidgetPrivate::setKeysInterpolation(const std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >& keys,Natron::KeyframeType type){
    
    CurveEditor* editor = NULL;
    if (_widget->parentWidget()) {
        if (_widget->parentWidget()->parentWidget()) {
            if (_widget->parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName) {
                editor = dynamic_cast<CurveEditor*>(_widget->parentWidget()->parentWidget());
            }
        }
    }
    
    //if the curve widget is the one owned by the curve editor use the undo/redo stack
    if(editor){
        if(keys.size() > 1){
            editor->setKeysInterpolation(keys, type);
        }else if(keys.size() == 1){
            editor->setKeyInterpolation(keys.front().first,keys.front().second,type);
        }
    }else{
        for(U32 i = 0; i < keys.size();++i){
            keys[i].first->getInternalCurve()->setKeyFrameInterpolation(keys[i].second->getInterpolation(), keys[i].second);
        }
        _widget->updateGL();
    }
}


///////////////////////////////////////////////////////////////////
// CurveWidget
//

CurveWidget::CurveWidget(boost::shared_ptr<TimeLine> timeline, QWidget* parent, const QGLWidget* shareWidget)
    : QGLWidget(parent,shareWidget)
    , _imp(new CurveWidgetPrivate(timeline,this))
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    if(timeline){
        QObject::connect(timeline.get(),SIGNAL(frameChanged(SequenceTime,int)),this,SLOT(onTimeLineFrameChanged(SequenceTime,int)));
        QObject::connect(timeline.get(),SIGNAL(boundariesChanged(SequenceTime,SequenceTime)),this,SLOT(onTimeLineBoundariesChanged(SequenceTime,SequenceTime)));
    }
}

CurveWidget::~CurveWidget(){
}

void CurveWidget::initializeGL(){
    
    if (!glewIsSupported("GL_ARB_vertex_array_object "  // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                         )) {
        _imp->_hasOpenGLVAOSupport = false;
    }
}

CurveGui* CurveWidget::createCurve(boost::shared_ptr<Curve> curve,const QString& name){
    updateGL(); //force initializeGL to be called if it wasn't before.
    CurveGui* curveGui = new CurveGui(this,curve,name,QColor(255,255,255),1);
    _imp->_curves.push_back(curveGui);
    curveGui->setColor(_imp->_nextCurveAddedColor);
    _imp->_nextCurveAddedColor.setHsv(_imp->_nextCurveAddedColor.hsvHue() + 60,
                                      _imp->_nextCurveAddedColor.hsvSaturation(),_imp->_nextCurveAddedColor.value());
    return curveGui;
}


void CurveWidget::removeCurve(CurveGui *curve){
    for(std::list<CurveGui* >::iterator it = _imp->_curves.begin();it!=_imp->_curves.end();++it){
        if((*it) == curve){
            //remove all its keyframes from selected keys
            const KeyFrameSet& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (KeyFrameSet::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                SelectedKeys::iterator foundSelected = _imp->_selectedKeyFrames.end();
                for(SelectedKeys::iterator it3 = _imp->_selectedKeyFrames.begin();it3!=_imp->_selectedKeyFrames.end();++it3){
                    if ((*it3)->key == *it2) {
                        foundSelected = it3;
                        break;
                    }
                }
                if(foundSelected != _imp->_selectedKeyFrames.end()){
                    _imp->_selectedKeyFrames.erase(foundSelected);
                }
            }
            delete (*it);
            _imp->_curves.erase(it);
            break;
        }
    }
}

void CurveWidget::centerOn(const std::vector<CurveGui*>& curves){
    RectD ret;
    for(U32 i = 0; i < curves.size();++i){
        CurveGui* c = curves[i];
        
        double xmin = c->getInternalCurve()->getMinimumTimeCovered();
        double xmax = c->getInternalCurve()->getMaximumTimeCovered();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        const KeyFrameSet& keys = c->getInternalCurve()->getKeyFrames();
        for (KeyFrameSet::const_iterator it2 = keys.begin(); it2!=keys.end(); ++it2) {
            const Variant& v = (*it2)->getValue();
            double value;
            if(v.type() == QVariant::Int){
                value = (double)v.toInt();
            }else{
                value = v.toDouble();
            }
            if(value < ymin)
                ymin = value;
            if(value > ymax)
                ymax = value;
        }
        ret.merge(xmin,ymin,xmax,ymax);
    }
    ret.set_bottom(ret.bottom() - ret.height()/10);
    ret.set_left(ret.left() - ret.width()/10);
    ret.set_right(ret.right() + ret.width()/10);
    ret.set_top(ret.top() + ret.height()/10);
    
    centerOn(ret.left(), ret.right(), ret.bottom(), ret.top());
}

void CurveWidget::showCurvesAndHideOthers(const std::vector<CurveGui*>& curves){
    for(std::list<CurveGui* >::iterator it = _imp->_curves.begin();it!=_imp->_curves.end();++it){
        std::vector<CurveGui*>::const_iterator it2 = std::find(curves.begin(), curves.end(), *it);
        if(it2 != curves.end()){
            (*it)->setVisible(true);
        }else{
            (*it)->setVisible(false);
        }
    }
    
    updateGL();
}

void CurveWidget::centerOn(double xmin,double xmax,double ymin,double ymax){
    double curveWidth = xmax - xmin;
    double curveHeight = (ymax - ymin);
    double w = width();
    double h = height() * _imp->_zoomCtx.aspectRatio ;
    if(w / h < curveWidth / curveHeight){
        _imp->_zoomCtx.left = xmin;
        _imp->_zoomCtx.zoomFactor = w / curveWidth;
        _imp->_zoomCtx.bottom = (ymax + ymin) / 2. - ((h / w) * curveWidth / 2.);
    } else {
        _imp->_zoomCtx.bottom = ymin;
        _imp->_zoomCtx.zoomFactor = h / curveHeight;
        _imp->_zoomCtx.left = (xmax + xmin) / 2. - ((w / h) * curveHeight / 2.);
    }
    
    refreshDisplayedTangents();

    updateGL();
}

void CurveWidget::resizeGL(int width,int height){
    if(height == 0)
        height = 1;
    glViewport (0, 0, width , height);
    centerOn(-10,500,-10,10);
}

void CurveWidget::paintGL()
{
    double w = (double)width();
    double h = (double)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    //assert(_zoomCtx._zoomFactor > 0);
    if(_imp->_zoomCtx.zoomFactor <= 0){
        return;
    }
    //assert(_zoomCtx._zoomFactor <= 1024);
    double bottom = _imp->_zoomCtx.bottom;
    double left = _imp->_zoomCtx.left;
    double top = bottom +  h / (double)_imp->_zoomCtx.zoomFactor * _imp->_zoomCtx.aspectRatio ;
    double right = left +  (w / (double)_imp->_zoomCtx.zoomFactor);
    if(left == right || top == bottom){
        glClearColor(_imp->_clearColor.redF(),_imp->_clearColor.greenF(),_imp->_clearColor.blueF(),_imp->_clearColor.alphaF());
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    _imp->_zoomCtx._lastOrthoLeft = left;
    _imp->_zoomCtx._lastOrthoRight = right;
    _imp->_zoomCtx._lastOrthoBottom = bottom;
    _imp->_zoomCtx._lastOrthoTop = top;
    glOrtho(left , right, bottom, top, -1, 1);
    checkGLErrors();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(_imp->_clearColor.redF(),_imp->_clearColor.greenF(),_imp->_clearColor.blueF(),_imp->_clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    
    _imp->drawScale();
    
    _imp->drawBaseAxis();

    if(_imp->_timelineEnabled){
        _imp->drawTimelineMarkers();
    }
    
    if(_imp->_drawSelectedKeyFramesBbox){
        _imp->drawSelectedKeyFramesBbox();
    }
    
    
    _imp->drawCurves();
    
    if(!_imp->_selectionRectangle.isNull()){
        _imp->drawSelectionRectangle();
    }

}

void CurveWidget::renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const
{
    assert(QGLContext::currentContext() == context());
    
    if(text.isEmpty())
        return;
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    QPointF pos = toWidgetCoordinates(x, y);
    _imp->_textRenderer.renderText(pos.x(),h-pos.y(),text,color,font);
    checkGLErrors();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(_imp->_zoomCtx._lastOrthoLeft,_imp->_zoomCtx._lastOrthoRight,_imp->_zoomCtx._lastOrthoBottom,_imp->_zoomCtx._lastOrthoTop,-1,1);
    glMatrixMode(GL_MODELVIEW);
    
}


//
// Decide what should be done in response to a mouse press.
// When the reason is found, process it and return.
// (this function has as many return points as there are reasons)
//
void CurveWidget::mousePressEvent(QMouseEvent *event) {

    ////
    // right button: popup menu
    if (event->button() == Qt::RightButton) {
        _imp->_rightClickMenu->exec(mapToGlobal(event->pos()));
        // no need to set _imp->_zoomCtx._oldClick
        // no need to set _imp->_dragStartPoint
        // no need to updateGL()
        return;
    }
    ////
    // middle button: scroll view
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _imp->_state = DRAGGING_VIEW;
        _imp->_zoomCtx._oldClick = event->pos();
        // no need to set _imp->_dragStartPoint
        // no need to updateGL()
        return;
    }

    // is the click near the multiple-keyframes selection box center?
    if( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget(event->pos())) {
        // yes, start dragging
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_KEYS;
        _imp->updateSelectedKeysMaxMovement();
        _imp->_zoomCtx._oldClick = event->pos();
        _imp->_dragStartPoint = event->pos();
        // no need to updateGL()
        return;
    }
    ////
    // is the click near a keyframe manipulator?
    std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > selectedKey = _imp->isNearbyKeyFrame(event->pos());
    if(selectedKey.second) {
        _imp->_drawSelectedKeyFramesBbox = false;
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_KEYS;
        setCursor(QCursor(Qt::CrossCursor));
        if (!event->modifiers().testFlag(Qt::ControlModifier)) {
            _imp->_selectedKeyFrames.clear();
        }
        boost::shared_ptr<SelectedKey> selected(new SelectedKey);
        selected->curve = selectedKey.first;
        selected->key = selectedKey.second;
        _imp->refreshKeyTangentsGUI(selected);
        _imp->_selectedKeyFrames.push_back(selected);
        _imp->updateSelectedKeysMaxMovement();
        _imp->_zoomCtx._oldClick = event->pos();
        _imp->_dragStartPoint = event->pos();
        updateGL(); // the keyframe changes color and the tangents must be drawn
        return;
    }
    ////
    // is the click near a tangent manipulator?
    std::pair<CurveGui::SelectedTangent,SelectedKeys::const_iterator > selectedTan = _imp->isNearByTangent(event->pos());
    
    //select the tangent only if it is not a constant keyframe
    if (selectedTan.second != _imp->_selectedKeyFrames.end() && (*selectedTan.second)->key->getInterpolation() != KEYFRAME_CONSTANT) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_TANGENT;
        _imp->_selectedTangent = selectedTan;
        _imp->_zoomCtx._oldClick = event->pos();
        //no need to set _imp->_dragStartPoint
        updateGL();
        return;
    }
    ////
    // is the click near the vertical current time marker?
    if(_imp->isNearbyTimelineBtmPoly(event->pos()) || _imp->isNearbyTimelineTopPoly(event->pos())) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_TIMELINE;
        _imp->_zoomCtx._oldClick = event->pos();
        // no need to set _imp->_dragStartPoint
        // no need to updateGL()
        return;
    }
    
    ////
    // is the click near a curve?
    Curves::const_iterator foundCurveNearby = _imp->isNearbyCurve(event->pos());
    if (foundCurveNearby != _imp->_curves.end()) {
        // yes, select it and don't start any other action, the user can then do per-curve specific actions
        // like centering on it on the viewport or pasting previously copied keyframes.
        // This is kind of the last resort action before the default behaviour (which is to draw
        // a selection rectangle), because we'd rather select a keyframe than the nearby curve
        _imp->selectCurve(*foundCurveNearby);
        updateGL();
        return;
    }
    
    ////
    // default behaviour: unselect selected keyframes, if any, and start a new selection
    _imp->_drawSelectedKeyFramesBbox = false;
    if(!event->modifiers().testFlag(Qt::ControlModifier)){
        _imp->_selectedKeyFrames.clear();
    }
    _imp->_state = SELECTING;
    _imp->_zoomCtx._oldClick = event->pos();
    _imp->_dragStartPoint = event->pos();
    updateGL();
}

void CurveWidget::mouseReleaseEvent(QMouseEvent*) {
    EventState prevState = _imp->_state;
    _imp->_state = NONE;
    _imp->_selectionRectangle.setBottomRight(QPointF(0,0));
    _imp->_selectionRectangle.setTopLeft(_imp->_selectionRectangle.bottomRight());
    if(_imp->_selectedKeyFrames.size() > 1){
        _imp->_drawSelectedKeyFramesBbox = true;
    }
    if (prevState == SELECTING) { // should other cases be considered?
        updateGL();
    }
}

void CurveWidget::mouseMoveEvent(QMouseEvent *event){

    //set cursor depending on the situation

    //find out if there is a nearby  tangent handle
    std::pair<CurveGui::SelectedTangent,SelectedKeys::const_iterator > selectedTan = _imp->isNearByTangent(event->pos());

    //if the selected keyframes rectangle is drawn and we're nearby the cross
    if( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget(event->pos())){
        setCursor(QCursor(Qt::SizeAllCursor));
    }
    else{
        //if there's a keyframe handle nearby
        std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > selectedKey = _imp->isNearbyKeyFrame(event->pos());

        //if there's a keyframe or tangent handle nearby set the cursor to cross
        if(selectedKey.second || selectedTan.second != _imp->_selectedKeyFrames.end()){
            setCursor(QCursor(Qt::CrossCursor));
        }else{

            //if we're nearby a timeline polygon, set cursor to horizontal displacement
            if(_imp->isNearbyTimelineBtmPoly(event->pos()) || _imp->isNearbyTimelineTopPoly(event->pos())){
                setCursor(QCursor(Qt::SizeHorCursor));
            }else{

                //default case
                setCursor(QCursor(Qt::ArrowCursor));
            }
        }
    }

    if (_imp->_state == NONE) {
        // nothing else to do
        return;
    }

    // after this point , only mouse dragging situations are handled
    assert(_imp->_state != NONE);

    if(_imp->_mustSetDragOrientation){
        QPointF diff(event->pos() - _imp->_dragStartPoint);
        double dist = diff.manhattanLength();
        if(dist > 5){
            if(std::abs(diff.x()) > std::abs(diff.y())){
                _imp->_mouseDragOrientation.setX(1);
                _imp->_mouseDragOrientation.setY(0);
            }else{
                _imp->_mouseDragOrientation.setX(0);
                _imp->_mouseDragOrientation.setY(1);
            }
            _imp->_mustSetDragOrientation = false;
        }
    }

    QPointF newClick_opengl = toScaleCoordinates(event->x(),event->y());
    QPointF oldClick_opengl = toScaleCoordinates(_imp->_zoomCtx._oldClick.x(),_imp->_zoomCtx._oldClick.y());
    
    switch(_imp->_state){

    case DRAGGING_VIEW:
        _imp->_zoomCtx.bottom += (oldClick_opengl.y() - newClick_opengl.y());
        _imp->_zoomCtx.left += (oldClick_opengl.x() - newClick_opengl.x());
        break;

    case DRAGGING_KEYS:
        if(!_imp->_mustSetDragOrientation){
            _imp->moveSelectedKeyFrames(oldClick_opengl,newClick_opengl);
        }
        break;

    case SELECTING:
        _imp->refreshSelectionRectangle((double)event->x(),(double)event->y());
        break;

    case DRAGGING_TANGENT:
        _imp->moveSelectedTangent(newClick_opengl);
        break;

    case DRAGGING_TIMELINE:
        _imp->_timeline->seekFrame((SequenceTime)newClick_opengl.x(),NULL);
        break;

    case NONE:
        assert(0);
        break;
    }

    _imp->_zoomCtx._oldClick = event->pos();
    
    updateGL();
}

void CurveWidget::refreshSelectedKeysBbox(){
    RectD keyFramesBbox;
    for(SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin() ; it!= _imp->_selectedKeyFrames.end();++it){
        double x = (*it)->key->getTime();
        double y = (*it)->key->getValue().value<double>();
        if(it != _imp->_selectedKeyFrames.begin()){
            if(x < keyFramesBbox.left()){
                keyFramesBbox.set_left(x);
            }
            if(x > keyFramesBbox.right()){
                keyFramesBbox.set_right(x);
            }
            if(y > keyFramesBbox.top()){
                keyFramesBbox.set_top(y);
            }
            if(y < keyFramesBbox.bottom()){
                keyFramesBbox.set_bottom(y);
            }
        }else{
            keyFramesBbox.set_left(x);
            keyFramesBbox.set_right(x);
            keyFramesBbox.set_top(y);
            keyFramesBbox.set_bottom(y);
        }
    }
    QPointF topLeft(keyFramesBbox.left(),keyFramesBbox.top());
    QPointF btmRight(keyFramesBbox.right(),keyFramesBbox.bottom());
    _imp->_selectedKeyFramesBbox.setTopLeft(topLeft);
    _imp->_selectedKeyFramesBbox.setBottomRight(btmRight);

    QPointF middle((topLeft.x() + btmRight.x()) / 2., (topLeft.y() + btmRight.y()) / 2. );
    QPointF middleWidgetCoord = toWidgetCoordinates(middle.x(),middle.y());
    QPointF middleLeft = toScaleCoordinates(middleWidgetCoord.x() - 20,middleWidgetCoord.y());
    QPointF middleRight = toScaleCoordinates(middleWidgetCoord.x() + 20,middleWidgetCoord.y());
    QPointF middleTop = toScaleCoordinates(middleWidgetCoord.x() ,middleWidgetCoord.y()-20);
    QPointF middleBottom = toScaleCoordinates(middleWidgetCoord.x(),middleWidgetCoord.y()+20);

    _imp->_selectedKeyFramesCrossHorizLine.setPoints(middleLeft,middleRight);
    _imp->_selectedKeyFramesCrossVertLine.setPoints(middleBottom,middleTop);
}

void CurveWidget::wheelEvent(QWheelEvent *event) {
    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (event->orientation() != Qt::Vertical) {
        return;
    }

    const double oldAspectRatio = _imp->_zoomCtx.aspectRatio;
    const double oldZoomFactor = _imp->_zoomCtx.zoomFactor;
    double newAspectRatio = oldAspectRatio;
    double newZoomFactor = oldZoomFactor;
    const double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());

    if (event->modifiers().testFlag(Qt::AltModifier) && event->modifiers().testFlag(Qt::ShiftModifier)) {
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        newAspectRatio *= scaleFactor;
        if (newAspectRatio <= 0.0001) {
            newAspectRatio = 0.0001;
        } else if (newAspectRatio > 10000.) {
            newAspectRatio = 10000.;
        }
    } else if (event->modifiers().testFlag(Qt::AltModifier)) {
        // Alt + Wheel: zoom time only, keep point under mouse
        newAspectRatio *= scaleFactor;
        newZoomFactor *= scaleFactor;
        if (newZoomFactor <= 0.0001) {
            newAspectRatio *= 0.0001/newZoomFactor;
            newZoomFactor = 0.0001;
        } else if (newZoomFactor > 10000.) {
            newAspectRatio *= 10000./newZoomFactor;
            newZoomFactor = 10000.;
        }
        if (newAspectRatio <= 0.0001) {
            newZoomFactor *= 0.0001/newAspectRatio;
            newAspectRatio = 0.0001;
        } else if (newAspectRatio > 10000.) {
            newZoomFactor *= 10000./newAspectRatio;
            newAspectRatio = 10000.;
        }
    } else  {
        // Wheel: zoom values and time, keep point under mouse
        newZoomFactor *= scaleFactor;
        if (newZoomFactor <= 0.0001) {
            newZoomFactor = 0.0001;
        } else if (newZoomFactor > 10000.) {
            newZoomFactor = 10000.;
        }
    }
    QPointF zoomCenter = toScaleCoordinates(event->x(), event->y());
    double zoomRatio =  oldZoomFactor / newZoomFactor;
    double aspectRatioRatio =  oldAspectRatio / newAspectRatio;
    _imp->_zoomCtx.left = zoomCenter.x() - (zoomCenter.x() - _imp->_zoomCtx.left)*zoomRatio ;
    _imp->_zoomCtx.bottom = zoomCenter.y() - (zoomCenter.y() - _imp->_zoomCtx.bottom)*zoomRatio/aspectRatioRatio;

    _imp->_zoomCtx.aspectRatio = newAspectRatio;
    _imp->_zoomCtx.zoomFactor = newZoomFactor;
    
    refreshDisplayedTangents();
    
    updateGL();
}

QPointF CurveWidget::toScaleCoordinates(double x,double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->_zoomCtx.bottom;
    double left = _imp->_zoomCtx.left;
    double top =  bottom +  h / _imp->_zoomCtx.zoomFactor * _imp->_zoomCtx.aspectRatio ;
    double right = left +  w / _imp->_zoomCtx.zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPointF CurveWidget::toWidgetCoordinates(double x, double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->_zoomCtx.bottom;
    double left = _imp->_zoomCtx.left;
    double top =  bottom +  h / _imp->_zoomCtx.zoomFactor * _imp->_zoomCtx.aspectRatio ;
    double right = left +  w / _imp->_zoomCtx.zoomFactor;
    return QPointF(((x - left)/(right - left))*w,((y - top)/(bottom - top))*h);
}

QSize CurveWidget::sizeHint() const{
    return QSize(1000,1000);
}

void CurveWidget::addKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key){
    curve->getInternalCurve()->addKeyFrame(key);
    if(curve->getInternalCurve()->isAnimated()){
        curve->setVisibleAndRefresh(true);
    }
    //if the keyframe is selected, refresh its tangents
    for (SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it) {
        if ((*it)->key == key) {
            _imp->refreshKeyTangentsGUI(*it);
            break;
        }
    }
}

boost::shared_ptr<KeyFrame> CurveWidget::addKeyFrame(CurveGui* curve,const Variant& y, int x){
    boost::shared_ptr<KeyFrame> key(new KeyFrame(x,y));
    addKeyFrame(curve,key);
    return key;
}

void CurveWidget::removeKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key){
    curve->getInternalCurve()->removeKeyFrame(key);
    if(!curve->getInternalCurve()->isAnimated()){
        curve->setVisibleAndRefresh(false);
    }
    for (SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it!=_imp->_selectedKeyFrames.end(); ++it) {
        if((*it)->key == key){
            _imp->_selectedKeyFrames.erase(it);
            break;
        }
    }
}

void CurveWidget::keyPressEvent(QKeyEvent *event){
    if(event->key() == Qt::Key_Space){
        if(parentWidget()){
            if(parentWidget()->parentWidget()){
                if(parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName){
                    QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
                    QCoreApplication::postEvent(parentWidget()->parentWidget(),ev);
                }
            }
        }
        
    }
    else if(event->key() == Qt::Key_Backspace){
        deleteSelectedKeyFrames();
    }else if(event->key() == Qt::Key_K){
        constantInterpForSelectedKeyFrames();
    }
    else if(event->key() == Qt::Key_L){
        linearInterpForSelectedKeyFrames();
    }
    else if(event->key() == Qt::Key_Z){
        smoothForSelectedKeyFrames();
    }
    else if(event->key() == Qt::Key_R){
        catmullromInterpForSelectedKeyFrames();
    }
    else if(event->key() == Qt::Key_C && event->modifiers().testFlag(Qt::NoModifier)){
        cubicInterpForSelectedKeyFrames();
    }
    else if(event->key() == Qt::Key_H){
        horizontalInterpForSelectedKeyFrames();
    }else if(event->key() == Qt::Key_X){
        breakTangentsForSelectedKeyFrames();
    }else if(event->key() == Qt::Key_F){
        frameSelectedCurve();
    }else if(event->key() == Qt::Key_A && event->modifiers().testFlag(Qt::ControlModifier)){
        selectAllKeyFrames();
    }else if(event->key() == Qt::Key_C && event->modifiers().testFlag(Qt::ControlModifier)){
        copySelectedKeyFrames();
    }else if(event->key() == Qt::Key_V && event->modifiers().testFlag(Qt::ControlModifier)){
        pasteKeyFramesFromClipBoardToSelectedCurve();
    }
    
    
}


void CurveWidget::enterEvent(QEvent */*event*/){
    setFocus();
}

void CurveWidget::refreshDisplayedTangents(){
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        _imp->refreshKeyTangentsGUI(*it);
    }
    updateGL();
}



void CurveWidget::constantInterpForSelectedKeyFrames(){
    
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_CONSTANT);
}

void CurveWidget::linearInterpForSelectedKeyFrames(){
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_LINEAR);
}

void CurveWidget::smoothForSelectedKeyFrames(){
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_SMOOTH);
}

void CurveWidget::catmullromInterpForSelectedKeyFrames(){
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_CATMULL_ROM);
}

void CurveWidget::cubicInterpForSelectedKeyFrames(){
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_CUBIC);
}

void CurveWidget::horizontalInterpForSelectedKeyFrames(){
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_HORIZONTAL);
}

void CurveWidget::breakTangentsForSelectedKeyFrames(){
    std::vector<std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >keys;
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        keys.push_back(std::make_pair((*it)->curve,(*it)->key));
    }
    _imp->setKeysInterpolation(keys,KEYFRAME_BROKEN);
}

void CurveWidget::deleteSelectedKeyFrames(){
    if(_imp->_selectedKeyFrames.empty())
        return;
    
    _imp->_drawSelectedKeyFramesBbox = false;
    _imp->_selectedKeyFramesBbox.setBottomRight(QPointF(0,0));
    _imp->_selectedKeyFramesBbox.setTopLeft(_imp->_selectedKeyFramesBbox.bottomRight());
    CurveEditor* editor = NULL;
    if(parentWidget()){
        if(parentWidget()->parentWidget()){
            if(parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName){
                editor = dynamic_cast<CurveEditor*>(parentWidget()->parentWidget());
                
            }
        }
    }
    
    if(editor){
        
        //if the parent is the editor, it will call removeKeyFrame but also add it to the undo stack
        if(_imp->_selectedKeyFrames.size() > 1){
            std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > > toRemove;
            for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin();it!=_imp->_selectedKeyFrames.end();++it){
                toRemove.push_back(std::make_pair((*it)->curve,(*it)->key));
            }
            editor->removeKeyFrames(toRemove);
            
        }else{
            boost::shared_ptr<SelectedKey>  it = _imp->_selectedKeyFrames.front();
            editor->removeKeyFrame(it->curve,it->key);
        }
        _imp->_selectedKeyFrames.clear();
        
    }else{
        while(!_imp->_selectedKeyFrames.empty()){
            boost::shared_ptr<SelectedKey> it = _imp->_selectedKeyFrames.back();
            removeKeyFrame(it->curve,it->key);
        }
    }
    
    updateGL();
}

void CurveWidget::copySelectedKeyFrames(){
    _imp->_keyFramesClipBoard.clear();
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        _imp->_keyFramesClipBoard.push_back(std::make_pair((*it)->key->getTime(),(*it)->key->getValue()));
    }
}
void CurveWidget::pasteKeyFramesFromClipBoardToSelectedCurve(){
    CurveEditor* editor = NULL;
    if(parentWidget()){
        if(parentWidget()->parentWidget()){
            if(parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName){
                editor = dynamic_cast<CurveEditor*>(parentWidget()->parentWidget());
                
            }
        }
    }
    CurveGui* curve = NULL;
    for (Curves::iterator it = _imp->_curves.begin() ; it != _imp->_curves.end() ; ++it) {
        if((*it)->isSelected()){
            curve = (*it);
            break;
        }
    }
    if(!curve){
        warningDialog("Curve Editor","You must select a curve first.");
        return;
    }
    std::vector<std::pair<SequenceTime,Variant> > keys;
    for(U32 i = 0; i < _imp->_keyFramesClipBoard.size();++i){
        std::pair<double,Variant>& toCopy = _imp->_keyFramesClipBoard[i];
        if(!editor){
            addKeyFrame(curve,toCopy.second,toCopy.first);
        }else{
            keys.push_back(toCopy);
        }
    }
    if(editor){
        editor->addKeyFrames(curve, keys);
    }else{
        updateGL();
    }
}

void CurveWidget::selectAllKeyFrames(){
    _imp->_drawSelectedKeyFramesBbox = true;
    for (Curves::iterator it = _imp->_curves.begin() ; it != _imp->_curves.end() ; ++it) {
        if((*it)->isVisible()){
            const KeyFrameSet& keys = (*it)->getInternalCurve()->getKeyFrames();
            for( KeyFrameSet::const_iterator it2 = keys.begin(); it2!=keys.end();++it2){

                std::pair<SelectedKeys::const_iterator,bool> found = isKeySelected(*it2);
                if(!found.second){
                    boost::shared_ptr<SelectedKey> newSelected(new SelectedKey);
                    newSelected->key = *it2;
                    newSelected->curve = *it;
                    _imp->refreshKeyTangentsGUI(newSelected);
                    _imp->_selectedKeyFrames.push_back(newSelected);
                }
            }
        }
    }
    refreshSelectedKeysBbox();
    updateGL();
}

void CurveWidget::frameSelectedCurve() {
    for (Curves::iterator it = _imp->_curves.begin() ; it != _imp->_curves.end() ; ++it) {
        if((*it)->isSelected()){
            std::vector<CurveGui*> curves;
            curves.push_back(*it);
            centerOn(curves);
            return;
        }
    }
    warningDialog("Curve Editor","You must select a curve first.");
}

void CurveWidget::onTimeLineFrameChanged(SequenceTime,int /*reason*/){
    if(!_imp->_timelineEnabled){
        _imp->_timelineEnabled = true;
    }
    _imp->refreshTimelinePositions();
    updateGL();
}
void CurveWidget::onTimeLineBoundariesChanged(SequenceTime,SequenceTime){
    updateGL();
}



std::pair<SelectedKeys::const_iterator,bool> CurveWidget::isKeySelected(boost::shared_ptr<KeyFrame> key) const{
    for(SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin();it!=_imp->_selectedKeyFrames.end();++it){
        if((*it)->key == key){
            return std::make_pair(it,true);
        }
    }
    return std::make_pair(_imp->_selectedKeyFrames.end(),false);
}

const QColor& CurveWidget::getSelectedCurveColor() const { return _imp->_selectedCurveColor; }

const QFont& CurveWidget::getFont() const { return *_imp->_font; }

const SelectedKeys& CurveWidget::getSelectedKeyFrames() const { return _imp->_selectedKeyFrames; }

bool CurveWidget::isSupportingOpenGLVAO() const { return _imp->_hasOpenGLVAOSupport; }


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
#include "Gui/CurveEditorUndoRedo.h"

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


struct SelectedKey_belongs_to_curve{
    const CurveGui* _curve;
    SelectedKey_belongs_to_curve(const CurveGui* curve) : _curve(curve){}

    bool operator() (const SelectedKey& k) const {
        return k.curve == _curve;
    }
};
}

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
    
    QObject::connect(this,SIGNAL(curveChanged()),curveWidget,SLOT(update()));
    
}

CurveGui::~CurveGui(){
    
}

std::pair<KeyFrame,bool> CurveGui::nextPointForSegment(double x1, double* x2){
    const KeyFrameSet& keys = _internalCurve->getKeyFrames();
    assert(!keys.empty());
    double xminCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keys.begin()->getTime(),0).x();
    double xmaxCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keys.rbegin()->getTime(),0).x();
    if(x1 < xminCurveWidgetCoord){
        *x2 = xminCurveWidgetCoord;
    }else if(x1 >= xmaxCurveWidgetCoord){
        *x2 = _curveWidget->width() - 1;
    }else {
        //we're between 2 keyframes,get the upper and lower
        KeyFrameSet::const_iterator upper = keys.end();
        double upperWidgetCoord = x1;
        for(KeyFrameSet::const_iterator it = keys.begin();it!=keys.end();++it){
            upperWidgetCoord = _curveWidget->toWidgetCoordinates(it->getTime(),0).x();
            if(upperWidgetCoord > x1){
                upper = it;
                break;
            }
        }
        assert(upper != keys.end() && upper!= keys.begin());
        
        KeyFrameSet::const_iterator lower = upper;
        --lower;
        
        double t = ( x1 - lower->getTime() ) / (upper->getTime() - lower->getTime());
        double P3 = upper->getValue();
        double P0 = lower->getValue();
        // Hermite coefficients P0' and P3' are for t normalized in [0,1]
        double P3pl = upper->getLeftDerivative() / (upper->getTime() - lower->getTime()); // normalize for t \in [0,1]
        double P0pr = lower->getRightDerivative() / (upper->getTime() - lower->getTime()); // normalize for t \in [0,1]
        double secondDer = 6. * (1. - t) *(P3 - P3pl / 3. - P0 - 2. * P0pr / 3.) +
                6.* t * (P0 - P3 + 2 * P3pl / 3. + P0pr / 3. );
        double secondDerWidgetCoord = std::abs(_curveWidget->toWidgetCoordinates(0,secondDer).y()
                                               / (upper->getTime() - lower->getTime()));
        // compute delta_x so that the y difference between the derivative and the curve is at most
        // 1 pixel (use the second order Taylor expansion of the function)
        double delta_x = std::max(2. / std::max(std::sqrt(secondDerWidgetCoord),0.1), 1.);
        
        if(upperWidgetCoord < x1 + delta_x){
            *x2 = upperWidgetCoord;
            return std::make_pair(*upper,true);
        }else{
            *x2 = x1 + delta_x;
        }
    }
    return std::make_pair(KeyFrame(0,0),false);

    
}

void CurveGui::drawCurve(){
    if(!_visible)
        return;
    
    assert(QGLContext::currentContext() == _curveWidget->context());
    
    std::vector<float> vertices;
    double x1 = 0;
    double x2;
    double w = _curveWidget->width();
    std::pair<KeyFrame,bool> isX1AKey;
    while(x1 < (w -1)){
        double x,y;
        if(!isX1AKey.second){
            x = _curveWidget->toScaleCoordinates(x1,0).x();
            y = evaluate(x);
        }else{
            x = isX1AKey.first.getTime();
            y = isX1AKey.first.getValue();
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
        const KeyFrame& key = (*k);
        //if the key is selected change its color to white
        SelectedKeys::const_iterator isSelected = selectedKeyFrames.end();
        for(SelectedKeys::const_iterator it2 = selectedKeyFrames.begin();
            it2 != selectedKeyFrames.end();++it2){
            if(it2->key.getTime() == key.getTime() && it2->curve == this){
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
        if(isSelected != selectedKeyFrames.end() && key.getInterpolation() != KEYFRAME_CONSTANT){

            //draw the derivatives lines
            if(key.getInterpolation() != KEYFRAME_FREE && key.getInterpolation() != KEYFRAME_BROKEN){
                glLineStipple(2, 0xAAAA);
                glEnable(GL_LINE_STIPPLE);
            }
            glBegin(GL_LINES);
            glColor4f(1., 0.35, 0.35, 1.);
            glVertex2f(isSelected->leftTan.first, isSelected->leftTan.second);
            glVertex2f(x, y);
            glVertex2f(x, y);
            glVertex2f(isSelected->rightTan.first, isSelected->rightTan.second);
            glEnd();
            if(key.getInterpolation() != KEYFRAME_FREE && key.getInterpolation() != KEYFRAME_BROKEN){
                glDisable(GL_LINE_STIPPLE);
            }
            
            if(selectedKeyFrames.size() == 1){ //if one keyframe, also draw the coordinates
                QString coordStr("x: %1, y: %2");
                coordStr = coordStr.arg(x).arg(y);
                double yWidgetCoord = _curveWidget->toWidgetCoordinates(0,key.getValue()).y();
                QFontMetrics m(_curveWidget->getFont());
                yWidgetCoord += (m.height() + 4);
                glColor4f(1., 1., 1., 1.);
                checkGLFrameBuffer();
                _curveWidget->renderText(x, _curveWidget->toScaleCoordinates(0, yWidgetCoord).y(),
                                         coordStr, QColor(240,240,240), _curveWidget->getFont());
                
            }
            glBegin(GL_POINTS);
            glVertex2f(isSelected->leftTan.first, isSelected->leftTan.second);
            glVertex2f(isSelected->rightTan.first, isSelected->rightTan.second);
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
    return _internalCurve->getValueAt(x);
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


    std::pair<CurveGui*,KeyFrame> isNearbyKeyFrame(const QPoint& pt) const;

    std::pair<CurveGui::SelectedDerivative, SelectedKey> isNearbyTangent(const QPoint& pt) const;

    bool isNearbySelectedKeyFramesCrossWidget(const QPoint& pt) const;

    bool isNearbyTimelineTopPoly(const QPoint& pt) const;

    bool isNearbyTimelineBtmPoly(const QPoint& pt) const;

    /**
     * @brief Selects the curve given in parameter and deselects any other curve in the widget.
     **/
    void selectCurve(CurveGui* curve);

    void moveSelectedKeyFrames(const QPointF& oldClick_opengl,const QPointF& newClick_opengl);

    void moveSelectedTangent(const QPointF& pos);
    
    void refreshKeyTangents(SelectedKey* key);

    void refreshSelectionRectangle(double x,double y);

    void updateSelectedKeysMaxMovement();
    
    void setSelectedKeysInterpolation(Natron::KeyframeType type);

private:

    void createMenu();

    void keyFramesWithinRect(const QRectF& rect,std::vector< std::pair<CurveGui*,KeyFrame > >* keys) const;

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

    std::vector< KeyFrame > _keyFramesClipBoard;
    QRectF _selectionRectangle;
    QPointF _dragStartPoint;
    bool _drawSelectedKeyFramesBbox;
    QRectF _selectedKeyFramesBbox;
    QLineF _selectedKeyFramesCrossVertLine;
    QLineF _selectedKeyFramesCrossHorizLine;
    boost::shared_ptr<TimeLine> _timeline;
    bool _timelineEnabled;
    std::pair<CurveGui::SelectedDerivative,SelectedKey> _selectedDerivative;
    
private:

    QColor _baseAxisColor;
    QColor _scaleColor;
    QPoint _keyDragMaxMovement;
    int _keyDragLastMovement;
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
    , _selectedDerivative()
    , _baseAxisColor(118,215,90,255)
    , _scaleColor(67,123,52,255)
    , _keyDragMaxMovement()
    , _keyDragLastMovement(0.)
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


    QAction* breakDerivatives = new QAction("Break",_widget);
    breakDerivatives->setShortcut(QKeySequence(Qt::Key_X));
    QObject::connect(breakDerivatives,SIGNAL(triggered()),_widget,SLOT(breakDerivativesForSelectedKeyFrames()));
    interpMenu->addAction(breakDerivatives);

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


std::pair<CurveGui*,KeyFrame> CurveWidgetPrivate::isNearbyKeyFrame(const QPoint& pt) const {
    for (Curves::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        if ((*it)->isVisible()) {
            const KeyFrameSet& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (KeyFrameSet::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                QPointF keyFramewidgetPos = _widget->toWidgetCoordinates(it2->getTime(), it2->getValue());
                if ((std::abs(pt.y() - keyFramewidgetPos.y()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                        (std::abs(pt.x() - keyFramewidgetPos.x()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
                    return std::make_pair(*it,*it2);
                }
            }
        }
    }
    return std::make_pair((CurveGui*)NULL,KeyFrame());
}

std::pair<CurveGui::SelectedDerivative,SelectedKey > CurveWidgetPrivate::isNearbyTangent(const QPoint& pt) const {
    for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it!=_selectedKeyFrames.end(); ++it) {
        QPointF leftTanPt = _widget->toWidgetCoordinates(it->leftTan.first,it->leftTan.second);
        QPointF rightTanPt = _widget->toWidgetCoordinates(it->rightTan.first,it->rightTan.second);
        if (pt.x() >= (leftTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                pt.x() <= (leftTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                pt.y() <= (leftTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                pt.y() >= (leftTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
            return std::make_pair(CurveGui::LEFT_TANGENT,*it);
        } else if (pt.x() >= (rightTanPt.x() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   pt.x() <= (rightTanPt.x() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   pt.y() <= (rightTanPt.y() + CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   pt.y() >= (rightTanPt.y() - CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)) {
            return std::make_pair(CurveGui::RIGHT_TANGENT,*it);
        }

    }
    return std::make_pair(CurveGui::LEFT_TANGENT,SelectedKey());
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


void CurveWidgetPrivate::keyFramesWithinRect(const QRectF& rect,std::vector< std::pair<CurveGui*,KeyFrame > >* keys) const {

    double left = rect.topLeft().x();
    double right = rect.bottomRight().x();
    double bottom = rect.topLeft().y();
    double top = rect.bottomRight().y();
    for (Curves::const_iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        const KeyFrameSet& keyframes = (*it)->getInternalCurve()->getKeyFrames();
        for (KeyFrameSet::const_iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
            double y = it2->getValue();
            int x = it2->getTime() ;
            if (x <= right && x >= left && y <= top && y >= bottom) {
                keys->push_back(std::make_pair(*it,*it2));
            }
        }
    }
}

struct MoveKey_functor{
    bool _moveX;
    double _lastKeyDrag;
    double _totalDrag;
    double _yTranslation;

    MoveKey_functor(bool moveX,double lastKeyDrag,double totalDrag,double yTranslation)
        : _moveX(moveX)
        , _lastKeyDrag(lastKeyDrag)
        , _totalDrag(totalDrag)
        , _yTranslation(yTranslation)
    {}

    SelectedKey operator()(SelectedKey key){
        double newX;
        if (_moveX) {
            newX = key.key.getTime() + _totalDrag - _lastKeyDrag;
        } else {
            newX = key.key.getTime();
        }
        double newY = key.key.getValue() + _yTranslation;
        key.key = key.curve->getInternalCurve()->setKeyFrameValueAndTime(
                    newX,newY, key.key.getTime());
        return key;
    }
};

void CurveWidgetPrivate::moveSelectedKeyFrames(const QPointF& oldClick_opengl,const QPointF& newClick_opengl) {
    QPointF dragStartPointOpenGL = _widget->toScaleCoordinates(_dragStartPoint.x(),_dragStartPoint.y());
    QPointF translation = (newClick_opengl - oldClick_opengl);
    translation.rx() *= _mouseDragOrientation.x();
    translation.ry() *= _mouseDragOrientation.y();

    //the CurveEditor is reponsible for the undo/redo. In case this is the curveWidget's that belongs to the
    //curve editor, use its undo/redo stack
    CurveEditor* editor = NULL;
    if (_widget->parentWidget()) {
        if (_widget->parentWidget()->parentWidget()) {
            if (_widget->parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName) {
                editor = dynamic_cast<CurveEditor*>(_widget->parentWidget()->parentWidget());
            }
        }
    }


    //1st off, round to the nearest integer the keyframes total motion
    int totalMovement = std::floor(newClick_opengl.x() - dragStartPointOpenGL.x() + 0.5);
    // clamp totalMovement to _keyDragMaxMovement
    if (totalMovement < 0) {
        totalMovement = std::max(totalMovement,_keyDragMaxMovement.x());
    } else {
        totalMovement = std::min(totalMovement,_keyDragMaxMovement.y());
    }

    if(!editor){
        //no editor for undo/redo, just set keyframes positions 1 by 1
        SelectedKeys copy;
        std::transform(_selectedKeyFrames.begin(),_selectedKeyFrames.end(),
                       std::inserter(copy,copy.begin()),
                       MoveKey_functor(_mouseDragOrientation.x() != 0,_keyDragLastMovement
                                       ,totalMovement,translation.y()));
        _selectedKeyFrames = copy;
        _widget->refreshSelectedKeysBbox();
        //refresh now the derivatives positions
        _widget->refreshDisplayedTangents();
    }else{

        //several keys, do a multiple move command
        std::vector<KeyMove> moves;
        int dt;
        if (_mouseDragOrientation.x() != 0) {
            dt =  totalMovement - _keyDragLastMovement;
        } else {
            dt = 0;
        }
        double dv = translation.y();

        if(dt != 0 || dv != 0){

            for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
                KeyMove move;
                move.curve = it->curve;
                move.oldPos = it->key;
                moves.push_back(move);
            }
            //the editor redo() call will call refreshSelectedKeysBbox() for us
            //and also call refreshDisplayedTangents()
            editor->setKeyFrames(moves,dt,dv);

        }

    }

    //update last drag movement
    if (_mouseDragOrientation.x() != 0) {
        _keyDragLastMovement = totalMovement;
    }
}

void CurveWidgetPrivate::moveSelectedTangent(const QPointF& pos) {

    SelectedKeys::iterator existingKey = _selectedKeyFrames.find(_selectedDerivative.second);
    assert(existingKey != _selectedKeyFrames.end());

    SelectedKey& key = _selectedDerivative.second;
    const KeyFrameSet& keys = key.curve->getInternalCurve()->getKeyFrames();

    
    KeyFrameSet::const_iterator cur = keys.find(key.key);
    assert(cur != keys.end());
    double dy = key.key.getValue() - pos.y();
    double dx = key.key.getTime() - pos.x();

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
    KeyframeType interp = key.key.getInterpolation();
    bool keyframeIsFirstOrLast = (prev == keys.end() || next == keys.end());
    bool interpIsNotBroken = (interp != KEYFRAME_BROKEN);
    bool interpIsCatmullRomOrCubicOrFree = (interp == KEYFRAME_CATMULL_ROM ||
                                            interp == KEYFRAME_CUBIC ||
                                            interp == KEYFRAME_FREE);
    bool setBothDerivative = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree : interpIsNotBroken;


    // For other keyframes:
    // - if they KEYFRAME_BROKEN, move only one derivative
    // - else change to KEYFRAME_FREE and move both derivatives
    if (setBothDerivative) {
        key.key = key.curve->getInternalCurve()->setKeyFrameInterpolation(KEYFRAME_FREE, key.key.getTime());

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
        key.key = key.curve->getInternalCurve()->setKeyFrameDerivatives(derivative, derivative, key.key.getTime());

    } else {
        key.key = key.curve->getInternalCurve()->setKeyFrameInterpolation(KEYFRAME_BROKEN, key.key.getTime());
        if (_selectedDerivative.first == CurveGui::LEFT_TANGENT) {
            //if dx is not of the good sign it would make the curve uncontrollable
            if (dx < 0) {
                dx = 0.0001;
            }

            double derivative = dy / dx;
            key.key = key.curve->getInternalCurve()->setKeyFrameLeftDerivative(derivative, key.key.getTime());

        } else {
            //if dx is not of the good sign it would make the curve uncontrollable
            if (dx > 0) {
                dx = -0.0001;
            }

            double derivative = dy / dx;
            key.key = key.curve->getInternalCurve()->setKeyFrameRightDerivative(derivative,key.key.getTime());
        }
    }
    refreshKeyTangents(&key);
    //erase the existing key in the set and replace it with the new one
    _selectedKeyFrames.erase(existingKey);
    std::pair<SelectedKeys::iterator,bool> ret = _selectedKeyFrames.insert(key);
    assert(ret.second);
    
    //also refresh prev/next derivatives gui if there're selected
    if(prev != keys.begin()){
        SelectedKey prevSelected(key.curve,*prev);
        SelectedKeys::const_iterator foundPrev = _selectedKeyFrames.find(prevSelected);
        if(foundPrev != _selectedKeyFrames.end()){
            refreshKeyTangents(&prevSelected);
            _selectedKeyFrames.erase(foundPrev);
            _selectedKeyFrames.insert(prevSelected);
        }
    }
    if(next != keys.end()){
        SelectedKey nextSelected(key.curve,*next);
        SelectedKeys::const_iterator foundNext = _selectedKeyFrames.find(nextSelected);
        if(foundNext != _selectedKeyFrames.end()){
            refreshKeyTangents(&nextSelected);
            _selectedKeyFrames.erase(foundNext);
            _selectedKeyFrames.insert(nextSelected);
        }

    }
    
}

void CurveWidgetPrivate::refreshKeyTangents(SelectedKey* key) {
    double w = (double)_widget->width();
    double h = (double)_widget->height();
    double x = key->key.getTime();
    double y = key->key.getValue();
    QPointF keyWidgetCoord = _widget->toWidgetCoordinates(x,y);
    const KeyFrameSet& keyframes = key->curve->getInternalCurve()->getKeyFrames();
    KeyFrameSet::const_iterator k = keyframes.find(key->key);
    assert(k != keyframes.end());

    //find the previous and next keyframes on the curve to find out the  position of the derivatives
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
        double prevTime = (prev == keyframes.end()) ? (x - 1.) : prev->getTime();
        double leftTan = key->key.getLeftDerivative();
        double leftTanXWidgetDiffMax = w / 8.;
        if (prev != keyframes.end()) {
            double prevKeyXWidgetCoord = _widget->toWidgetCoordinates(prevTime, 0).x();
            //set the left derivative X to be at 1/3 of the interval [prev,k], and clamp it to 1/8 of the widget width.
            leftTanXWidgetDiffMax = std::min(leftTanXWidgetDiffMax, (keyWidgetCoord.x() - prevKeyXWidgetCoord) / 3.);
        }
        //clamp the left derivative Y to 1/8 of the widget height.
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
        double nextTime = (next == keyframes.end()) ? (x + 1.) : next->getTime();
        double rightTan = key->key.getRightDerivative();
        double rightTanXWidgetDiffMax = w / 8.;
        if (next != keyframes.end()) {
            double nextKeyXWidgetCoord = _widget->toWidgetCoordinates(nextTime, 0).x();
            //set the right derivative X to be at 1/3 of the interval [k,next], and clamp it to 1/8 of the widget width.
            rightTanXWidgetDiffMax = std::min(rightTanXWidgetDiffMax, (nextKeyXWidgetCoord - keyWidgetCoord.x()) / 3.);
        }
        //clamp the right derivative Y to 1/8 of the widget height.
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
    _selectedKeyFrames.clear();
    std::vector< std::pair<CurveGui*,KeyFrame > > keyframesSelected;
    keyFramesWithinRect(_selectionRectangle,&keyframesSelected);
    for (U32 i = 0; i < keyframesSelected.size(); ++i) {
        SelectedKey newSelectedKey(keyframesSelected[i].first,keyframesSelected[i].second);
        refreshKeyTangents(&newSelectedKey);
        //insert it into the _selectedKeyFrames
        std::pair<SelectedKeys::iterator,bool> insertRet = _selectedKeyFrames.insert(newSelectedKey);
        if(!insertRet.second){
            _selectedKeyFrames.erase(insertRet.first);
            insertRet = _selectedKeyFrames.insert(newSelectedKey);
            assert(insertRet.second);
        }
        
    }

    //the CurveEditor is reponsible for the undo/redo. In case this is the curveWidget's that belongs to the
    //curve editor, use its undo/redo stack
    CurveEditor* editor = NULL;
    if (_widget->parentWidget()) {
        if (_widget->parentWidget()->parentWidget()) {
            if (_widget->parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName) {
                editor = dynamic_cast<CurveEditor*>(_widget->parentWidget()->parentWidget());
            }
        }
    }
    //we notify the undo stack of the editor that the selection changed and that it needs to stop merging
    //move commands.
    if(editor){
        std::vector<KeyMove> empty;
        editor->setKeyFrames(empty,0,0);
    }


    _widget->refreshSelectedKeysBbox();
}

void CurveWidgetPrivate::updateSelectedKeysMaxMovement() {
    if (_selectedKeyFrames.empty()) {
        return;
    }

    std::map<CurveGui*,QPoint> curvesMaxMovements;
    
    //for each curve that has keyframes selected,we want to find out the max movement possible on left/right
    // that means looking at the first selected key and last selected key of each curve and determining of
    //how much they can move
    for(SelectedKeys::const_iterator it = _selectedKeyFrames.begin();it!=_selectedKeyFrames.end();++it){
        
        std::map<CurveGui*,QPoint>::iterator foundCurveMovement = curvesMaxMovements.find(it->curve);
        
        if(foundCurveMovement != curvesMaxMovements.end()){
            //if we already computed the max movement for this curve, move on
            continue;
        }else{
            
            const KeyFrameSet& ks = it->curve->getInternalCurve()->getKeyFrames();
            
            //find out in this set what is the first key selected
            SelectedKeys::const_iterator leftMostSelected = std::find_if(
                        _selectedKeyFrames.begin(),_selectedKeyFrames.end(),SelectedKey_belongs_to_curve(it->curve));

            // watch out this is a reverse iterator
            SelectedKeys::const_reverse_iterator rightMostSelected = std::find_if(
                        _selectedKeyFrames.rbegin(),_selectedKeyFrames.rend(),SelectedKey_belongs_to_curve(it->curve));
            
            //there must be a left most and right most! but they can be the same key
            assert(leftMostSelected != _selectedKeyFrames.end() && rightMostSelected != _selectedKeyFrames.rend());

            KeyFrameSet::const_iterator leftMost = ks.find(leftMostSelected->key);
            KeyFrameSet::const_iterator rightMost = ks.find(rightMostSelected->key);

            assert(leftMost != ks.end() && rightMost != ks.end());

            QPoint curveMaxMovement;
            //now get leftMostSelected's previous key to determine the max left movement for this curve
            {
                if(leftMost == ks.begin()){
                    curveMaxMovement.setX(INT_MIN);
                }else{
                    KeyFrameSet::const_iterator prev = leftMost;
                    --prev;
                    curveMaxMovement.setX(prev->getTime() + 1 - leftMost->getTime());
                    assert(curveMaxMovement.x() <= 0);
                }
            }
            
            //now get rightMostSelected's next key to determine the max right movement for this curve
            {
                KeyFrameSet::const_iterator next = rightMost;
                ++next;
                if(next == ks.end()){
                    curveMaxMovement.setY(INT_MAX);
                }else{

                    curveMaxMovement.setY(next->getTime() - 1 - rightMost->getTime());
                    assert(curveMaxMovement.y() >= 0);

                }
            }
            curvesMaxMovements.insert(std::make_pair(it->curve, curveMaxMovement));
        }
    }
    
    //last step, we need to get the minimum of all curveMaxMovements to determine what is the real
    //selected keys max movement
    
    _keyDragMaxMovement.rx() = INT_MIN;
    _keyDragMaxMovement.ry() = INT_MAX;
    
    
    for (std::map<CurveGui*,QPoint>::const_iterator it = curvesMaxMovements.begin(); it!= curvesMaxMovements.end(); ++it) {
        const QPoint& pt = it->second;
        assert(pt.x() <= 0 && _keyDragMaxMovement.x() <= 0);
        //get the minimum for the left movement (numbers are all negatives here)
        if(pt.x() > _keyDragMaxMovement.x()){
            _keyDragMaxMovement.setX(pt.x());
        }
        
        assert(pt.y() >= 0 && _keyDragMaxMovement.y() >= 0);
        //get the minimum for the right movement (numbers are all positives here)
        if(pt.y() < _keyDragMaxMovement.y()){
            _keyDragMaxMovement.setY(pt.y());
        }
        
    }
    assert(_keyDragMaxMovement.x() <= 0 && _keyDragMaxMovement.y() >= 0);

    _keyDragLastMovement = 0.;

}


struct ChangeInterpolation_functor{
    Natron::KeyframeType _type;
    ChangeInterpolation_functor(Natron::KeyframeType type)
        : _type(type)
    {}

    SelectedKey operator()(SelectedKey key){
        key.key = key.curve->getInternalCurve()->setKeyFrameInterpolation(_type,key.key.getTime());
        return key;
    }
};


void CurveWidgetPrivate::setSelectedKeysInterpolation(Natron::KeyframeType type){
    
    //if the parent widget is the editor, use its undo/redo stack.
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
        if(_selectedKeyFrames.size() > 1){
            //do a multiple change at once
            std::vector<KeyInterpolationChange> changes;
            for(SelectedKeys::iterator it = _selectedKeyFrames.begin();it!=_selectedKeyFrames.end();++it){
                KeyInterpolationChange change;
                change.curve = it->curve;
                change.oldInterp = it->key.getInterpolation();
                change.newInterp = type;
                change.key = it->key;
                changes.push_back(change);
            }
            editor->setKeysInterpolation(changes);
        }else if(_selectedKeyFrames.size() == 1){
            //just do 1 undo/redo change

            KeyInterpolationChange change;
            change.curve = _selectedKeyFrames.begin()->curve;
            change.oldInterp = _selectedKeyFrames.begin()->key.getInterpolation();
            change.newInterp = type;
            change.key = _selectedKeyFrames.begin()->key;
            editor->setKeyInterpolation(change);
        }
    }else{
        //just change interpolation 1 by 1
        SelectedKeys copy;
        std::transform(_selectedKeyFrames.begin(),_selectedKeyFrames.end(),
                       std::inserter(copy,copy.begin()),ChangeInterpolation_functor(type));
        _selectedKeyFrames = copy;
        _widget->refreshDisplayedTangents();
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
        QObject::connect(timeline.get(),SIGNAL(boundariesChanged(SequenceTime,SequenceTime,int)),this,SLOT(onTimeLineBoundariesChanged(SequenceTime,SequenceTime,int)));
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
    //updateGL(); //force initializeGL to be called if it wasn't before.
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
                SelectedKeys::iterator foundSelected = _imp->_selectedKeyFrames.find(SelectedKey(*it,*it2));
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
            double value = it2->getValue();
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
    
    update();
}

void CurveWidget::getVisibleCurves(std::vector<CurveGui*>* curves) const{
    for(std::list<CurveGui* >::iterator it = _imp->_curves.begin();it!=_imp->_curves.end();++it){
        if((*it)->isVisible()){
            curves->push_back(*it);
        }
    }
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

    update();
}

double CurveWidget::getPixelAspectRatio() const{
    return _imp->_zoomCtx.aspectRatio;
}

void CurveWidget::getBackgroundColor(double *r,double *g,double* b) const{
    *r = _imp->_clearColor.redF();
    *g = _imp->_clearColor.greenF();
    *b = _imp->_clearColor.blueF();
}

void CurveWidget::resizeGL(int width,int height){
    if(height == 0)
        height = 1;
    glViewport (0, 0, width , height);
    

    ///find out what are the selected curves and center on them
    std::vector<CurveGui*> curves;
    getVisibleCurves(&curves);
    if(curves.empty()){
        centerOn(-10,500,-10,10);
    }else{
        centerOn(curves);
    }
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
    std::pair<CurveGui*,KeyFrame > selectedKey = _imp->isNearbyKeyFrame(event->pos());
    if(selectedKey.first) {
        _imp->_drawSelectedKeyFramesBbox = false;
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_KEYS;
        setCursor(QCursor(Qt::CrossCursor));

        //get the previous selected key to determine whether we need to stop merging move commands
        SelectedKey previouslySelectedKey;
        bool hadASelectedKey = false;
        if(_imp->_selectedKeyFrames.size() == 1){
            previouslySelectedKey = *(_imp->_selectedKeyFrames.begin());
            hadASelectedKey = true;
        }



        if (!event->modifiers().testFlag(Qt::ControlModifier)) {
            _imp->_selectedKeyFrames.clear();
        }
        SelectedKey selected(selectedKey.first,selectedKey.second);
        
        _imp->refreshKeyTangents(&selected);

        bool mustStopMergingMoveCommands = hadASelectedKey && (
                    previouslySelectedKey.curve != selected.curve ||
                    previouslySelectedKey.key.getTime() != selected.key.getTime());


        //insert it into the _selectedKeyFrames
        std::pair<SelectedKeys::iterator,bool> insertRet = _imp->_selectedKeyFrames.insert(selected);
        if(!insertRet.second){
            _imp->_selectedKeyFrames.erase(insertRet.first);
            insertRet = _imp->_selectedKeyFrames.insert(selected);
            assert(insertRet.second);
        }

        if(mustStopMergingMoveCommands){
            //the CurveEditor is reponsible for the undo/redo. In case this is the curveWidget's that belongs to the
            //curve editor, use its undo/redo stack
            CurveEditor* editor = NULL;
            if (parentWidget()) {
                if (parentWidget()->parentWidget()) {
                    if (parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName) {
                        editor = dynamic_cast<CurveEditor*>(parentWidget()->parentWidget());
                    }
                }
            }
            //we notify the undo stack of the editor that the selection changed and that it needs to stop merging
            //move commands.
            if(editor){
                std::vector<KeyMove> empty;
                editor->setKeyFrames(empty,0,0);
            }
        }

        _imp->updateSelectedKeysMaxMovement();
        _imp->_zoomCtx._oldClick = event->pos();
        _imp->_dragStartPoint = event->pos();
        update(); // the keyframe changes color and the derivatives must be drawn
        return;
    }
    ////
    // is the click near a derivative manipulator?
    std::pair<CurveGui::SelectedDerivative,SelectedKey > selectedTan = _imp->isNearbyTangent(event->pos());
    
    //select the derivative only if it is not a constant keyframe
    if (selectedTan.second.curve && selectedTan.second.key.getInterpolation() != KEYFRAME_CONSTANT) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = DRAGGING_TANGENT;
        _imp->_selectedDerivative = selectedTan;
        _imp->_zoomCtx._oldClick = event->pos();
        //no need to set _imp->_dragStartPoint
        update();
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
        update();
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
    update();
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
        update();
    }
}

void CurveWidget::mouseMoveEvent(QMouseEvent *event){

    //set cursor depending on the situation

    //find out if there is a nearby  derivative handle
    std::pair<CurveGui::SelectedDerivative,SelectedKey > selectedTan = _imp->isNearbyTangent(event->pos());

    //if the selected keyframes rectangle is drawn and we're nearby the cross
    if( _imp->_drawSelectedKeyFramesBbox && _imp->isNearbySelectedKeyFramesCrossWidget(event->pos())){
        setCursor(QCursor(Qt::SizeAllCursor));
    }
    else{
        //if there's a keyframe handle nearby
        std::pair<CurveGui*,KeyFrame > selectedKey = _imp->isNearbyKeyFrame(event->pos());

        //if there's a keyframe or derivative handle nearby set the cursor to cross
        if(selectedKey.first || selectedTan.second.curve){
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
    
    update();
}

void CurveWidget::refreshSelectedKeysBbox(){
    RectD keyFramesBbox;
    for(SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin() ; it!= _imp->_selectedKeyFrames.end();++it){
        double x = it->key.getTime();
        double y = it->key.getValue();
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

    if (event->modifiers().testFlag(Qt::ControlModifier) && event->modifiers().testFlag(Qt::ShiftModifier)) {
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        newAspectRatio *= scaleFactor;
        if (newAspectRatio <= 0.0001) {
            newAspectRatio = 0.0001;
        } else if (newAspectRatio > 10000.) {
            newAspectRatio = 10000.;
        }
    } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
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
    
    if(_imp->_drawSelectedKeyFramesBbox){
        refreshSelectedKeysBbox();
    }
    refreshDisplayedTangents();
    
    update();
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

void CurveWidget::addKeyFrame(CurveGui* curve,const KeyFrame& key){
    curve->getInternalCurve()->addKeyFrame(key);
    if(curve->getInternalCurve()->isAnimated()){
        curve->setVisibleAndRefresh(true);
    }
}

void CurveWidget::removeKeyFrame(CurveGui* curve,const KeyFrame& key){
    curve->getInternalCurve()->removeKeyFrame(key.getTime());
    if(!curve->getInternalCurve()->isAnimated()){
        curve->setVisibleAndRefresh(false);
    }
    SelectedKeys::iterator it = _imp->_selectedKeyFrames.find(SelectedKey(curve,key));
    if(it != _imp->_selectedKeyFrames.end()){
        _imp->_selectedKeyFrames.erase(it);
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
        breakDerivativesForSelectedKeyFrames();
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

struct RefreshTangent_functor{
    CurveWidgetPrivate* _imp;

    RefreshTangent_functor(CurveWidgetPrivate* imp): _imp(imp){}

    SelectedKey operator()(SelectedKey key){
        _imp->refreshKeyTangents(&key);
        return key;
    }
};

void CurveWidget::refreshDisplayedTangents(){
    SelectedKeys copy;
    std::transform( _imp->_selectedKeyFrames.begin(), _imp->_selectedKeyFrames.end(),
                    std::inserter(copy,copy.begin()),RefreshTangent_functor(_imp.get()));

    _imp->_selectedKeyFrames = copy;
    update();
}

void CurveWidget::setSelectedKeys(const SelectedKeys& keys){
    _imp->_selectedKeyFrames = keys;
}

void CurveWidget::constantInterpForSelectedKeyFrames(){  
    _imp->setSelectedKeysInterpolation(KEYFRAME_CONSTANT);
}

void CurveWidget::linearInterpForSelectedKeyFrames(){ 
    _imp->setSelectedKeysInterpolation(KEYFRAME_LINEAR);
}

void CurveWidget::smoothForSelectedKeyFrames(){
    _imp->setSelectedKeysInterpolation(KEYFRAME_SMOOTH);
}

void CurveWidget::catmullromInterpForSelectedKeyFrames(){
    _imp->setSelectedKeysInterpolation(KEYFRAME_CATMULL_ROM);
}

void CurveWidget::cubicInterpForSelectedKeyFrames(){
    _imp->setSelectedKeysInterpolation(KEYFRAME_CUBIC);
}

void CurveWidget::horizontalInterpForSelectedKeyFrames(){
    _imp->setSelectedKeysInterpolation(KEYFRAME_HORIZONTAL);
}

void CurveWidget::breakDerivativesForSelectedKeyFrames(){
    _imp->setSelectedKeysInterpolation(KEYFRAME_BROKEN);
}

void CurveWidget::deleteSelectedKeyFrames(){
    if(_imp->_selectedKeyFrames.empty())
        return;
    
    _imp->_drawSelectedKeyFramesBbox = false;
    _imp->_selectedKeyFramesBbox.setBottomRight(QPointF(0,0));
    _imp->_selectedKeyFramesBbox.setTopLeft(_imp->_selectedKeyFramesBbox.bottomRight());
    
    
    //if there's an editor, get its pointer: this is the editor that controls the undo/redo's
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
        //if there're multiple selected keyframes, do a multiple remove command, otherwise a single remove command
        if(_imp->_selectedKeyFrames.size() > 1){
            std::vector< std::pair<CurveGui*,KeyFrame > > toRemove;
            for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin();it!=_imp->_selectedKeyFrames.end();++it){
                toRemove.push_back(std::make_pair(it->curve,it->key));
            }
            editor->removeKeyFrames(toRemove);
            
        }else{
            SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin();
            editor->removeKeyFrame(it->curve,it->key);
        }
        _imp->_selectedKeyFrames.clear();
        
    }else{
        //while the set is not empty just remove the keyframes. No undo/redo in this case
        while(!_imp->_selectedKeyFrames.empty()){
            SelectedKeys::const_iterator it = _imp->_selectedKeyFrames.begin();
            removeKeyFrame(it->curve,it->key);
        }
    }
    
    update();
}

void CurveWidget::copySelectedKeyFrames(){
    _imp->_keyFramesClipBoard.clear();
    for(SelectedKeys::iterator it = _imp->_selectedKeyFrames.begin(); it != _imp->_selectedKeyFrames.end();++it){
        _imp->_keyFramesClipBoard.push_back(it->key);
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
    if(!editor){
        for(U32 i = 0; i < _imp->_keyFramesClipBoard.size();++i){
            addKeyFrame(curve,_imp->_keyFramesClipBoard[i]);
            
        }
        update();
    }else{
        //this function will call updateGL() for us
        editor->addKeyFrames(curve, _imp->_keyFramesClipBoard);
    }

}

void CurveWidget::selectAllKeyFrames(){
    _imp->_drawSelectedKeyFramesBbox = true;
    _imp->_selectedKeyFrames.clear();
    for (Curves::iterator it = _imp->_curves.begin() ; it != _imp->_curves.end() ; ++it) {
        if((*it)->isVisible()){
            const KeyFrameSet& keys = (*it)->getInternalCurve()->getKeyFrames();
            for( KeyFrameSet::const_iterator it2 = keys.begin(); it2!=keys.end();++it2){
                SelectedKey newSelected(*it,*it2);
                _imp->refreshKeyTangents(&newSelected);

                std::pair<SelectedKeys::iterator,bool> insertRet = _imp->_selectedKeyFrames.insert(newSelected);
                if(!insertRet.second){
                    _imp->_selectedKeyFrames.erase(insertRet.first);
                    insertRet = _imp->_selectedKeyFrames.insert(newSelected);
                    assert(insertRet.second);
                }

            }
        }
    }
    refreshSelectedKeysBbox();
    update();
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
    update();
}

void CurveWidget::onTimeLineBoundariesChanged(SequenceTime,SequenceTime,int){
    update();
}


const QColor& CurveWidget::getSelectedCurveColor() const { return _imp->_selectedCurveColor; }

const QFont& CurveWidget::getFont() const { return *_imp->_font; }

const SelectedKeys& CurveWidget::getSelectedKeyFrames() const { return _imp->_selectedKeyFrames; }

bool CurveWidget::isSupportingOpenGLVAO() const { return _imp->_hasOpenGLVAOSupport; }


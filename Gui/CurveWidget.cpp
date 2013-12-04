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

#include "Global/AppManager.h"
#include "Engine/Knob.h"
#include "Engine/Rect.h"
#include "Engine/TimeLine.h"

#include "Gui/ScaleSlider.h"
#include "Gui/ticks.h"
#include "Gui/CurveEditor.h"

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
                                               // (in widget pixels)
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

static double ASPECT_RATIO = 0.1;
static double AXIS_MAX = 100000.;
static double AXIS_MIN = -100000.;


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
    const Curve::KeyFrames& keys = _internalCurve->getKeyFrames();
    assert(!keys.empty());
    double xminCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keys.front()->getTime(),0).x();
    double xmaxCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keys.back()->getTime(),0).x();
    if(x1 < xminCurveWidgetCoord){
        *x2 = xminCurveWidgetCoord;
    }else if(x1 >= xmaxCurveWidgetCoord){
        *x2 = _curveWidget->width() - 1;
    }else{
        //we're between 2 keyframes,get the upper and lower
        Curve::KeyFrames::const_iterator upper = keys.end();
        double upperWidgetCoord;
        for(Curve::KeyFrames::const_iterator it = keys.begin();it!=keys.end();++it){
            upperWidgetCoord = _curveWidget->toWidgetCoordinates((*it)->getTime(),0).x();
            if(upperWidgetCoord > x1){
                upper = it;
                break;
            }
        }
        assert(upper != keys.end() && upper!= keys.begin());
        
        Curve::KeyFrames::const_iterator lower = upper;
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
    double h = _curveWidget->height();
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
    const Curve::KeyFrames& keyframes = _internalCurve->getKeyFrames();
    glPointSize(7.f);
    glEnable(GL_POINT_SMOOTH);
    
    const std::list< std::pair< CurveGui*,boost::shared_ptr<KeyFrame> > >& selectedKeyFrames = _curveWidget->getSelectedKeyFrames();
    for(Curve::KeyFrames::const_iterator k = keyframes.begin();k!=keyframes.end();++k){
        glColor4f(_color.redF(), _color.greenF(), _color.blueF(), _color.alphaF());
        boost::shared_ptr<KeyFrame> key = (*k);
        //if the key is selected change its color to white
        std::list< std::pair< CurveGui*,boost::shared_ptr<KeyFrame> > >::const_iterator isSelected = selectedKeyFrames.end();
        for(std::list< std::pair< CurveGui*,boost::shared_ptr<KeyFrame> > >::const_iterator it2 = selectedKeyFrames.begin();
            it2 != selectedKeyFrames.end();++it2){
            if((*it2).second == key){
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
        if(isSelected != selectedKeyFrames.end()){ //if one keyframe, also draw the coordinates
            QPointF keyWidgetCoord = _curveWidget->toWidgetCoordinates(x, y);
            
            //DRAWING TANGENTS
            //find the previous and next keyframes on the curve to find out the  position of the tangents
            Curve::KeyFrames::const_iterator prev = k;
            if(k != keyframes.begin()){
                --prev;
            }else{
                prev = keyframes.end();
            }
            Curve::KeyFrames::const_iterator next = k;
            ++next;
            double leftTan = key->getLeftTangent().toDouble()/(x - (*prev)->getTime());
            double leftTanX = 0;
            double leftTanY = leftTan + y;
            if(prev != keyframes.end()){
                double prevKeyXWidgetCoord = _curveWidget->toWidgetCoordinates((*prev)->getTime(), 0).x();
                //set the left tangent X to be at 1/3 of the interval [prev,k], and clamp it to 1/8 of the widget width.
                double leftTanXWidgetDiffMax = std::min( w/8., (keyWidgetCoord.x() - prevKeyXWidgetCoord) / 3.);
                //clamp the left tangent Y to 1/8 of the widget height.
                double leftTanYWidgetDiffMax = std::min( h/8., leftTanXWidgetDiffMax);
                assert(leftTanXWidgetDiffMax >= 0.);
                assert(leftTanYWidgetDiffMax >= 0.);

                QPointF tanMax = _curveWidget->toScaleCoordinates(keyWidgetCoord.x() + leftTanXWidgetDiffMax, keyWidgetCoord.y() -leftTanYWidgetDiffMax) - QPointF(x,y);
                assert(tanMax.x() >= 0.);
                assert(tanMax.y() >= 0.);

                if (tanMax.x() * std::abs(leftTan) < tanMax.y()) {
                    leftTanX = x - tanMax.x();
                    leftTanY = y - tanMax.x() * leftTan;
                } else {
                    leftTanX = x - tanMax.y() / std::abs(leftTan);
                    leftTanY = y - tanMax.y() * (leftTan > 0 ? 1 : -1);
                }
                assert(std::abs(leftTanX - x) <= tanMax.x());
                assert(std::abs(leftTanY - y) <= tanMax.y());
            }
            double rightTan = key->getRightTangent().toDouble()/((*next)->getTime() - x );
            double rightTanX = 0.;
            double rightTanY = rightTan + y;
            if(next != keyframes.end()){
                double nextKeyXWidgetCoord = _curveWidget->toWidgetCoordinates((*next)->getTime(), 0).x();
                //set the right tangent X to be at 1/3 of the interval [k,next], and clamp it to 1/8 of the widget width.
                double rightTanXWidgetDiffMax = std::min( w/8., (nextKeyXWidgetCoord - keyWidgetCoord.x()) / 3.);
                //clamp the right tangent Y to 1/8 of the widget height.
                double rightTanYWidgetDiffMax = std::min( h/8., rightTanXWidgetDiffMax);
                assert(rightTanXWidgetDiffMax >= 0.);
                assert(rightTanYWidgetDiffMax >= 0.);

                QPointF tanMax = _curveWidget->toScaleCoordinates(keyWidgetCoord.x() + rightTanXWidgetDiffMax, keyWidgetCoord.y() -rightTanYWidgetDiffMax) - QPointF(x,y);
                assert(tanMax.x() >= 0.);
                assert(tanMax.y() >= 0.);

                if (tanMax.x() * std::abs(rightTan) < tanMax.y()) {
                    rightTanX = x + tanMax.x();
                    rightTanY = y + tanMax.x() * rightTan;
                } else {
                    rightTanX = x + tanMax.y() / std::abs(rightTan);
                    rightTanY = y + tanMax.y() * (rightTan > 0 ? 1 : -1);
                }
                assert(std::abs(rightTanX - x) <= tanMax.x());
                assert(std::abs(rightTanY - y) <= tanMax.y());
            }
            
            //draw the tangents lines
            if(key->getInterpolation() != Natron::KEYFRAME_FREE && key->getInterpolation() != Natron::KEYFRAME_BROKEN){
                glLineStipple(2, 0xAAAA);
                glEnable(GL_LINE_STIPPLE);
            }
            glBegin(GL_LINES);
            glColor4f(1., 0.35, 0.35, 1.);
            if(prev != keyframes.end() && (*prev)->getInterpolation() != Natron::KEYFRAME_NONE){
                glVertex2f(leftTanX, leftTanY);
                glVertex2f(x, y);
            }
            if(next != keyframes.end() && (*next)->getInterpolation() != Natron::KEYFRAME_NONE){
                glVertex2f(x, y);
                glVertex2f(rightTanX, rightTanY);
            }
            glEnd();
            if(key->getInterpolation() != Natron::KEYFRAME_FREE && key->getInterpolation() != Natron::KEYFRAME_BROKEN){
                glDisable(GL_LINE_STIPPLE);
            }
            
            if(selectedKeyFrames.size() == 1){
                QString coordStr("x: %1, y: %2");
                coordStr = coordStr.arg(x).arg(y);
                double yWidgetCoord = keyWidgetCoord.y();
                QFontMetrics m(_curveWidget->getFont());
                yWidgetCoord += (m.height() + 4);
                glColor4f(1., 1., 1., 1.);
                _curveWidget->renderText(x, _curveWidget->toScaleCoordinates(0, yWidgetCoord).y(),
                                         coordStr, QColor(240,240,240), _curveWidget->getFont());
                
            }
            glBegin(GL_POINTS);
            if(prev != keyframes.end() && (*prev)->getInterpolation() != Natron::KEYFRAME_NONE){
                glVertex2f(leftTanX, leftTanY);
            }
            if(next != keyframes.end() && (*next)->getInterpolation() != Natron::KEYFRAME_NONE){
                glVertex2f(rightTanX, rightTanY);
            }
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

CurveWidget::CurveWidget(boost::shared_ptr<TimeLine> timeline, QWidget* parent, const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _zoomCtx()
, _state(NONE)
, _rightClickMenu(new QMenu(this))
, _clearColor(0,0,0,255)
, _baseAxisColor(118,215,90,255)
, _scaleColor(67,123,52,255)
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
, _drawSelectedKeyFramesBbox(false)
, _selectedKeyFramesBbox()
, _timeline(timeline)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);
    _nextCurveAddedColor.setHsv(200,255,255);
    createMenu();

    if(_timeline){
        QObject::connect(_timeline.get(),SIGNAL(frameChanged(SequenceTime,int)),this,SLOT(onTimeLineFrameChanged(SequenceTime,int)));
        QObject::connect(_timeline.get(),SIGNAL(boundariesChanged(SequenceTime,SequenceTime)),this,SLOT(onTimeLineBoundariesChanged(SequenceTime,SequenceTime)));
    }
}

CurveWidget::~CurveWidget(){
    delete _font;
    for(std::list<CurveGui*>::const_iterator it = _curves.begin();it!=_curves.end();++it){
        delete (*it);
    }
    _curves.clear();
}

void CurveWidget::initializeGL(){
    
    if (!glewIsSupported("GL_ARB_vertex_array_object "  // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                         )) {
        _hasOpenGLVAOSupport = false;
    }
}

CurveGui* CurveWidget::createCurve(boost::shared_ptr<Curve> curve,const QString& name){
    updateGL(); //force initializeGL to be called if it wasn't before.
    CurveGui* curveGui = new CurveGui(this,curve,name,QColor(255,255,255),1);
    _curves.push_back(curveGui);
    curveGui->setColor(_nextCurveAddedColor);
    _nextCurveAddedColor.setHsv(_nextCurveAddedColor.hsvHue() + 60,_nextCurveAddedColor.hsvSaturation(),_nextCurveAddedColor.value());
    return curveGui;
}


void CurveWidget::removeCurve(CurveGui *curve){
    for(std::list<CurveGui* >::iterator it = _curves.begin();it!=_curves.end();++it){
        if((*it) == curve){
            //remove all its keyframes from selected keys
            const Curve::KeyFrames& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (Curve::KeyFrames::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                SelectedKeys::iterator foundSelected = _selectedKeyFrames.end();
                for(SelectedKeys::iterator it3 = _selectedKeyFrames.begin();it3!=_selectedKeyFrames.end();++it3){
                    if (it3->second == *it2) {
                        foundSelected = it3;
                        break;
                    }
                }
                if(foundSelected != _selectedKeyFrames.end()){
                    _selectedKeyFrames.erase(foundSelected);
                }
            }
            delete (*it);
            _curves.erase(it);
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
        const Curve::KeyFrames& keys = c->getInternalCurve()->getKeyFrames();
        for (Curve::KeyFrames::const_iterator it2 = keys.begin(); it2!=keys.end(); ++it2) {
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
    for(std::list<CurveGui* >::iterator it = _curves.begin();it!=_curves.end();++it){
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
    double h = height() * ASPECT_RATIO ;
    if(w / h < curveWidth / curveHeight){
        _zoomCtx._left = xmin;
        _zoomCtx._zoomFactor = w / curveWidth;
        _zoomCtx._bottom = (ymax + ymin) / 2. - ((h / w) * curveWidth / 2.);
    } else {
        _zoomCtx._bottom = ymin;
        _zoomCtx._zoomFactor = h / curveHeight;
        _zoomCtx._left = (xmax + xmin) / 2. - ((w / h) * curveHeight / 2.);
    }
    
    
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
    if(_zoomCtx._zoomFactor <= 0){
        return;
    }
    //assert(_zoomCtx._zoomFactor <= 1024);
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top = bottom +  h / (double)_zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  (w / (double)_zoomCtx._zoomFactor);
    if(left == right || top == bottom){
        glClearColor(_clearColor.redF(),_clearColor.greenF(),_clearColor.blueF(),_clearColor.alphaF());
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    _zoomCtx._lastOrthoLeft = left;
    _zoomCtx._lastOrthoRight = right;
    _zoomCtx._lastOrthoBottom = bottom;
    _zoomCtx._lastOrthoTop = top;
    glOrtho(left , right, bottom, top, -1, 1);
    checkGLErrors();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(_clearColor.redF(),_clearColor.greenF(),_clearColor.blueF(),_clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    
    drawScale();
    
    drawBaseAxis();

    if(_timeline){
        drawTimelineMarkers();
    }
    
    if(_drawSelectedKeyFramesBbox){
        drawSelectedKeyFramesBbox();
    }
    
    
    drawCurves();
    
    if(!_selectionRectangle.isNull()){
        drawSelectionRectangle();
    }
    
}

void CurveWidget::drawSelectionRectangle(){
    
    glPushAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    
    glColor4f(0.3,0.3,0.3,0.2);
    QPointF btmRight =_selectionRectangle.bottomRight();
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

void CurveWidget::drawTimelineMarkers(){
    assert(_timeline);
    SequenceTime first = _timeline->leftBound();
    SequenceTime last = _timeline->rightBound();
    SequenceTime current = _timeline->currentFrame();

    QPointF topLeft = toScaleCoordinates(0,0);
    QPointF btmRight = toScaleCoordinates(width()-1,height()-1);

    glPushAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glColor4f(0.8,0.3,0.,1.);

    glBegin(GL_LINES);
    glVertex2f(first,btmRight.y());
    glVertex2f(first,topLeft.y());
    glVertex2f(last,btmRight.y());
    glVertex2f(last,topLeft.y());
    glColor4f(0.95,0.58,0.,1.);
    glVertex2f(current,btmRight.y());
    glVertex2f(current,topLeft.y());
    glEnd();

    glDisable(GL_LINE_SMOOTH);

    QPointF btmCursorBtm(current,btmRight.y());
    QPointF btmcursorBtmWidgetCoord = toWidgetCoordinates(btmCursorBtm.x(),btmCursorBtm.y());
    QPointF btmCursorTop = toScaleCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - CURSOR_HEIGHT);
    QPointF btmCursorLeft = toScaleCoordinates(btmcursorBtmWidgetCoord.x() - CURSOR_WIDTH /2, btmcursorBtmWidgetCoord.y());
    QPointF btmCursorRight = toScaleCoordinates(btmcursorBtmWidgetCoord.x() + CURSOR_WIDTH / 2,btmcursorBtmWidgetCoord.y());

    QPointF topCursortop(current,topLeft.y());
    QPointF topcursorTopWidgetCoord = toWidgetCoordinates(topCursortop.x(),topCursortop.y());
    QPointF topCursorBtm = toScaleCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + CURSOR_HEIGHT);
    QPointF topCursorLeft = toScaleCoordinates(topcursorTopWidgetCoord.x() - CURSOR_WIDTH /2, topcursorTopWidgetCoord.y());
    QPointF topCursorRight = toScaleCoordinates(topcursorTopWidgetCoord.x() + CURSOR_WIDTH / 2,topcursorTopWidgetCoord.y());

    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_DONT_CARE);

    glBegin(GL_POLYGON);
    glVertex2f(btmCursorLeft.x(),btmCursorLeft.y());
    glVertex2f(btmCursorTop.x(),btmCursorTop.y());
    glVertex2f(btmCursorRight.x(),btmCursorRight.y());
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(topCursorBtm.x(),topCursorBtm.y());
    glVertex2f(topCursorLeft.x(),topCursorLeft.y());
    glVertex2f(topCursorRight.x(),topCursorRight.y());
    glEnd();

    glDisable(GL_POLYGON_SMOOTH);
    glPopAttrib();
}

void CurveWidget::drawSelectedKeyFramesBbox(){
    
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
    
    QPointF middle((topLeft.x() + btmRight.x()) / 2., (topLeft.y() + btmRight.y()) / 2. );
    QPointF middleWidgetCoord = toWidgetCoordinates(middle.x(),middle.y());
    QPointF middleLeft = toScaleCoordinates(middleWidgetCoord.x() - 20,middleWidgetCoord.y());
    QPointF middleRight = toScaleCoordinates(middleWidgetCoord.x() + 20,middleWidgetCoord.y());
    QPointF middleTop = toScaleCoordinates(middleWidgetCoord.x() ,middleWidgetCoord.y()-20);
    QPointF middleBottom = toScaleCoordinates(middleWidgetCoord.x(),middleWidgetCoord.y()+20);
    
    glBegin(GL_LINES);
    glVertex2f(std::max(middleLeft.x(),topLeft.x()),middleLeft.y());
    glVertex2f(std::min(middleRight.x(),btmRight.x()),middleRight.y());
    glVertex2f(middleBottom.x(),std::max(middleBottom.y(),btmRight.y()));
    glVertex2f(middleTop.x(),std::min(middleTop.y(),topLeft.y()));
    glEnd();
    
    glDisable(GL_LINE_SMOOTH);
    checkGLErrors();
    
    glLineWidth(1.);
    glPopAttrib();
    glColor4f(1., 1., 1., 1.);
}

void CurveWidget::drawCurves()
{
    assert(QGLContext::currentContext() == context());
    //now draw each curve
    for(std::list<CurveGui*>::const_iterator it = _curves.begin();it!=_curves.end();++it){
        (*it)->drawCurve();
    }
}


void CurveWidget::drawBaseAxis()
{
    assert(QGLContext::currentContext() == context());
    
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

void CurveWidget::drawScale()
{
    assert(QGLContext::currentContext() == context());
    
    QPointF btmLeft = toScaleCoordinates(0,height()-1);
    QPointF topRight = toScaleCoordinates(width()-1, 0);
    
    
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
        const double rangePixel = (axis == 0) ? width() : height(); // AXIS-SPECIFIC
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
        for(int i = m1 ; i <= m2; ++i) {
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
                        renderText(value, btmLeft.y(), s, c, *_font); // AXIS-SPECIFIC
                    } else {
                        renderText(btmLeft.x(), value, s, c, *_font); // AXIS-SPECIFIC
                    }
                }
            }
        }
        
    }
    
    glDisable(GL_BLEND);
    //reset back the color
    glColor4f(1., 1., 1., 1.);
    
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
    _textRenderer.renderText(pos.x(),h-pos.y(),text,color,font);
    checkGLErrors();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(_zoomCtx._lastOrthoLeft,_zoomCtx._lastOrthoRight,_zoomCtx._lastOrthoBottom,_zoomCtx._lastOrthoTop,-1,1);
    glMatrixMode(GL_MODELVIEW);
    
}

CurveWidget::Curves::const_iterator CurveWidget::isNearbyCurve(const QPoint &pt) const{
    QPointF openGL_pos = toScaleCoordinates(pt.x(),pt.y());
    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
        if((*it)->isVisible()){
            double y = (*it)->evaluate(openGL_pos.x());
            double yWidget = toWidgetCoordinates(0,y).y();
            if(std::abs(pt.y() - yWidget) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE){
                return it;
            }
        }
    }
    return _curves.end();
}

std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > CurveWidget::isNearbyKeyFrame(const QPoint& pt) const{
    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
        if((*it)->isVisible()){
            const Curve::KeyFrames& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (Curve::KeyFrames::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                QPointF keyFramewidgetPos = toWidgetCoordinates((*it2)->getTime(), (*it2)->getValue().toDouble());
                if((std::abs(pt.y() - keyFramewidgetPos.y()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   (std::abs(pt.x() - keyFramewidgetPos.x()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)){
                    return std::make_pair((*it),(*it2));
                }
            }
        }
    }
    return std::make_pair((CurveGui*)NULL,boost::shared_ptr<KeyFrame>());
}

std::pair<CurveGui::SelectedTangent,boost::shared_ptr<KeyFrame> > CurveWidget::isNearByTangent(const QPoint& pt) const{
    //todo
//    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
//        if((*it)->isVisible()){
//            const Curve::KeyFrames& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
//            for (Curve::KeyFrames::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
//                
//            }
//            
//        }
//    }
}

void CurveWidget::selectCurve(CurveGui* curve){
    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
        (*it)->setSelected(false);
    }
    curve->setSelected(true);
}

void CurveWidget::mousePressEvent(QMouseEvent *event){
    _mustSetDragOrientation = true;
    
    if(event->button() == Qt::RightButton){
        _rightClickMenu->exec(mapToGlobal(event->pos()));
        return;
    }
    
    CurveWidget::Curves::const_iterator foundCurveNearby = isNearbyCurve(event->pos());
    if(foundCurveNearby != _curves.end()){
        selectCurve(*foundCurveNearby);
    }
    
    int x = event->x();
    int y = event->y();
    std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > selectedKey = isNearbyKeyFrame(event->pos());
    QPointF middle((_selectedKeyFramesBbox.topLeft().x() + _selectedKeyFramesBbox.bottomRight().x()) / 2.,
                   (_selectedKeyFramesBbox.topLeft().y() + _selectedKeyFramesBbox.bottomRight().y()) / 2.);
    QPointF middleWidget = toWidgetCoordinates(middle.x(), middle.y());
    if( _drawSelectedKeyFramesBbox &&
       (((x > (middleWidget.x() - 20) && x < (middleWidget.x() + 20)) && (y < (middleWidget.y() + 20) && y > (middleWidget.y() - 20))) ||
        (selectedKey.second))){
           _state = DRAGGING_KEYS;
       }
    else{
        
        
        
        if(selectedKey.second){
            _drawSelectedKeyFramesBbox = false;
            _state = DRAGGING_KEYS;
            setCursor(QCursor(Qt::CrossCursor));
            if(!event->modifiers().testFlag(Qt::ControlModifier)){
                _selectedKeyFrames.clear();
                
            }
            _selectedKeyFrames.push_back(selectedKey);
        }else{
            
            if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
                _state = DRAGGING_VIEW;
            }else{
                _drawSelectedKeyFramesBbox = false;
                if(!event->modifiers().testFlag(Qt::ControlModifier)){
                    _selectedKeyFrames.clear();
                    
                }
                _state = SELECTING;
            }
        }
    }
    _zoomCtx._oldClick = event->pos();
    _selectionStartPoint = event->pos();
    updateGL();
}

void CurveWidget::mouseReleaseEvent(QMouseEvent*){
    _state = NONE;
    _selectionRectangle.setBottomRight(QPointF(0,0));
    _selectionRectangle.setTopLeft(_selectionRectangle.bottomRight());
    if(_selectedKeyFrames.size() > 1){
        _drawSelectedKeyFramesBbox = true;
        
    }
    
    updateGL();
}
void CurveWidget::mouseMoveEvent(QMouseEvent *event){
    std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > selectedKey = isNearbyKeyFrame(event->pos());
    int x = event->x();
    int y = event->y();
    QPointF middle((_selectedKeyFramesBbox.topLeft().x() + _selectedKeyFramesBbox.bottomRight().x()) / 2.,
                   (_selectedKeyFramesBbox.topLeft().y() + _selectedKeyFramesBbox.bottomRight().y()) / 2.);
    QPointF middleWidget = toWidgetCoordinates(middle.x(), middle.y());
    if( _drawSelectedKeyFramesBbox &&
       ( (x > (middleWidget.x() - 20) && x < (middleWidget.x() + 20) && y < (middleWidget.y() + 20) && y > (middleWidget.y() - 20) ) ||
        (selectedKey.second))){
           setCursor(QCursor(Qt::SizeAllCursor));
       }
    else{
        if(selectedKey.second){
            setCursor(QCursor(Qt::CrossCursor));
        }else{
            setCursor(QCursor(Qt::ArrowCursor));
        }
    }
    
    if(_mustSetDragOrientation && _state == DRAGGING_KEYS){
        int dist = QPoint(event->pos() - _zoomCtx._oldClick).manhattanLength();
        if(dist > 5){
            if(std::abs(event->x() - _zoomCtx._oldClick.x()) > std::abs(event->y() - _zoomCtx._oldClick.y())){
                _mouseDragOrientation.setX(1);
                _mouseDragOrientation.setY(0);
            }else{
                _mouseDragOrientation.setX(0);
                _mouseDragOrientation.setY(1);
            }
            _mustSetDragOrientation = false;
        }else{
            return;
        }
    }
    QPoint newClick =  event->pos();
    QPointF newClick_opengl = toScaleCoordinates(newClick.x(),newClick.y());
    QPointF oldClick_opengl = toScaleCoordinates(_zoomCtx._oldClick.x(),_zoomCtx._oldClick.y());
    
    
    if (_state == DRAGGING_VIEW) {
        
        float dy = (oldClick_opengl.y() - newClick_opengl.y());
        _zoomCtx._bottom += dy;
        _zoomCtx._left += (oldClick_opengl.x() - newClick_opengl.x());
    }else if(_state == DRAGGING_KEYS){
        
        QPointF translation = (newClick_opengl - oldClick_opengl);
        translation.rx() *= _mouseDragOrientation.x();
        translation.ry() *= _mouseDragOrientation.y();
        
        
        CurveEditor* editor = NULL;
        if(parentWidget()){
            if(parentWidget()->parentWidget()){
                if(parentWidget()->parentWidget()->objectName() == kCurveEditorObjectName){
                    editor = dynamic_cast<CurveEditor*>(parentWidget()->parentWidget());
                    
                }
            }
        }
        std::vector<std::pair< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> >, std::pair<double, Variant> > > moves;

        for (SelectedKeys::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
            double newTime = newClick_opengl.x();//(*it)->getTime() + translation.x();
            double diffTime = (newTime - oldClick_opengl.x()) * _mouseDragOrientation.x() ;
            double newX,newY;
            if(diffTime != 0){
                //find out if the new value will be in the interval [previousKey,nextKey]
                double newValue = (*it).second->getTime() + std::ceil(diffTime);
                const Curve::KeyFrames& keys = (*it).first->getInternalCurve()->getKeyFrames();
                Curve::KeyFrames::const_iterator foundKey = std::find(keys.begin(), keys.end(), (*it).second);
                assert(foundKey != keys.end());
                Curve::KeyFrames::const_iterator prevKey = foundKey;
                if(foundKey != keys.begin()){
                    --prevKey;
                }else{
                    prevKey = keys.end();
                }
                Curve::KeyFrames::const_iterator nextKey = foundKey;
                ++nextKey;
                
                if(((prevKey != keys.end() && newValue > (*prevKey)->getTime()) || prevKey == keys.end()) &&
                   ((nextKey != keys.end() && newValue < (*nextKey)->getTime()) || nextKey == keys.end())){
                    newX = newValue;
                }else if(prevKey != keys.end() && newValue <= (*prevKey)->getTime()){
                    newX = (*prevKey)->getTime()+1;
                }else if(nextKey != keys.end() && newValue >= (*nextKey)->getTime()){
                    newX = (*nextKey)->getTime()-1;
                }
                
            }else{
                newX = (*it).second->getTime();
            }
            newY = (*it).second->getValue().toDouble() + translation.y();
            if(!editor){
                setKeyPos((*it).second,newX,Variant(newY));
            }else{
                if(_selectedKeyFrames.size() > 1){
                    moves.push_back(std::make_pair(std::make_pair(it->first,it->second),std::make_pair(newX,Variant(newY))));
                }else{
                    editor->setKeyFrame((*it).second,newX,Variant(newY));
                }
            }
        }
        if(editor && _selectedKeyFrames.size() > 1){
            editor->setKeyFrames(moves);
        }
        refreshSelectedKeysBbox();
    }else if(_state == SELECTING){
        double xmin = std::min(_selectionStartPoint.x(),(double)event->x());
        double xmax = std::max(_selectionStartPoint.x(),(double)event->x());
        double ymin = std::min(_selectionStartPoint.y(),(double)event->y());
        double ymax = std::max(_selectionStartPoint.y(),(double)event->y());
        _selectionRectangle.setBottomRight(toScaleCoordinates(xmax,ymin));
        _selectionRectangle.setTopLeft(toScaleCoordinates(xmin,ymax));
        _selectedKeyFrames.clear();
        std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > > keyframesSelected;
        keyFramesWithinRect(_selectionRectangle,&keyframesSelected);
        for(U32 i = 0; i < keyframesSelected.size();++i){
            _selectedKeyFrames.push_back(keyframesSelected[i]);
            
        }
        refreshSelectedKeysBbox();
    }
    
    _zoomCtx._oldClick = newClick;
    
    updateGL();
}

void CurveWidget::refreshSelectedKeysBbox(){
    RectD keyFramesBbox;
    for(SelectedKeys::const_iterator it = _selectedKeyFrames.begin() ; it!= _selectedKeyFrames.end();++it){
        double x = it->second->getTime();
        double y = it->second->getValue().value<double>();
        if(it != _selectedKeyFrames.begin()){
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
    _selectedKeyFramesBbox.setTopLeft(QPointF(keyFramesBbox.left(),keyFramesBbox.top()));
    _selectedKeyFramesBbox.setBottomRight(QPointF(keyFramesBbox.right(),keyFramesBbox.bottom()));
}
void CurveWidget::wheelEvent(QWheelEvent *event){
    if (event->orientation() != Qt::Vertical) {
        return;
    }
    double newZoomFactor;
    if (event->delta() > 0) {
        newZoomFactor = _zoomCtx._zoomFactor*std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    } else {
        newZoomFactor = _zoomCtx._zoomFactor/std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, -event->delta());
    }
    if (newZoomFactor <= 0.01) {
        newZoomFactor = 0.01;
    } else if (newZoomFactor > 1024.) {
        newZoomFactor = 1024.;
    }
    QPointF zoomCenter = toScaleCoordinates(event->x(), event->y());
    double zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
    _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio ;
    _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    
    updateGL();
    
}

QPointF CurveWidget::toScaleCoordinates(double x,double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPointF CurveWidget::toWidgetCoordinates(double x, double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  w / _zoomCtx._zoomFactor;
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
}

boost::shared_ptr<KeyFrame> CurveWidget::addKeyFrame(CurveGui* curve,const Variant& y, int x){
    boost::shared_ptr<KeyFrame> key(new KeyFrame(x,y,curve->getInternalCurve().get()));
    addKeyFrame(curve,key);
    return key;
}

void CurveWidget::removeKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key){
    curve->getInternalCurve()->removeKeyFrame(key);
    if(!curve->getInternalCurve()->isAnimated()){
        curve->setVisibleAndRefresh(false);
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

void CurveWidget::setKeyPos(boost::shared_ptr<KeyFrame> key, double x, const Variant& y){
    key->setTimeAndValue(x,y);
}

void CurveWidget::createMenu(){
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
    
    QAction* deleteKeyFramesAction = new QAction("Delete selected keyframes",this);
    deleteKeyFramesAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    QObject::connect(deleteKeyFramesAction,SIGNAL(triggered()),this,SLOT(deleteSelectedKeyFrames()));
    editMenu->addAction(deleteKeyFramesAction);
    
    QAction* copyKeyFramesAction = new QAction("Copy selected keyframes",this);
    copyKeyFramesAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    QObject::connect(copyKeyFramesAction,SIGNAL(triggered()),this,SLOT(copySelectedKeyFrames()));
    editMenu->addAction(copyKeyFramesAction);
    
    QAction* pasteKeyFramesAction = new QAction("Paste to selected curve",this);
    pasteKeyFramesAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    QObject::connect(pasteKeyFramesAction,SIGNAL(triggered()),this,SLOT(pasteKeyFramesFromClipBoardToSelectedCurve()));
    editMenu->addAction(pasteKeyFramesAction);
    
    QAction* selectAllAction = new QAction("Select all keyframes",this);
    selectAllAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    QObject::connect(selectAllAction,SIGNAL(triggered()),this,SLOT(selectAllKeyFrames()));
    editMenu->addAction(selectAllAction);
    
    
    QAction* constantInterp = new QAction("Constant",this);
    constantInterp->setShortcut(QKeySequence(Qt::Key_K));
    QObject::connect(constantInterp,SIGNAL(triggered()),this,SLOT(constantInterpForSelectedKeyFrames()));
    interpMenu->addAction(constantInterp);
    
    QAction* linearInterp = new QAction("Linear",this);
    linearInterp->setShortcut(QKeySequence(Qt::Key_L));
    QObject::connect(linearInterp,SIGNAL(triggered()),this,SLOT(linearInterpForSelectedKeyFrames()));
    interpMenu->addAction(linearInterp);
    
    
    QAction* smoothInterp = new QAction("Smooth",this);
    smoothInterp->setShortcut(QKeySequence(Qt::Key_Z));
    QObject::connect(smoothInterp,SIGNAL(triggered()),this,SLOT(smoothForSelectedKeyFrames()));
    interpMenu->addAction(smoothInterp);
    
    
    QAction* catmullRomInterp = new QAction("Catmull-Rom",this);
    catmullRomInterp->setShortcut(QKeySequence(Qt::Key_R));
    QObject::connect(catmullRomInterp,SIGNAL(triggered()),this,SLOT(catmullromInterpForSelectedKeyFrames()));
    interpMenu->addAction(catmullRomInterp);
    
    
    QAction* cubicInterp = new QAction("Cubic",this);
    cubicInterp->setShortcut(QKeySequence(Qt::Key_C));
    QObject::connect(cubicInterp,SIGNAL(triggered()),this,SLOT(cubicInterpForSelectedKeyFrames()));
    interpMenu->addAction(cubicInterp);
    
    QAction* horizontalInterp = new QAction("Horizontal",this);
    horizontalInterp->setShortcut(QKeySequence(Qt::Key_H));
    QObject::connect(horizontalInterp,SIGNAL(triggered()),this,SLOT(horizontalInterpForSelectedKeyFrames()));
    interpMenu->addAction(horizontalInterp);
    
    
    QAction* breakTangents = new QAction("Break",this);
    breakTangents->setShortcut(QKeySequence(Qt::Key_X));
    QObject::connect(breakTangents,SIGNAL(triggered()),this,SLOT(breakTangentsForSelectedKeyFrames()));
    interpMenu->addAction(breakTangents);
    
    QAction* frameCurve = new QAction("Frame selected curve",this);
    frameCurve->setShortcut(QKeySequence(Qt::Key_F));
    QObject::connect(frameCurve,SIGNAL(triggered()),this,SLOT(frameSelectedCurve()));
    viewMenu->addAction(frameCurve);
    
    
}

void CurveWidget::constantInterpForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_CONSTANT);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_CONSTANT);
        }
    }
    updateGL();
}

void CurveWidget::linearInterpForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_LINEAR);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_LINEAR);
        }
    }
    updateGL();
}

void CurveWidget::smoothForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_SMOOTH);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_SMOOTH);
        }
    }
    updateGL();
}

void CurveWidget::catmullromInterpForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_CATMULL_ROM);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_CATMULL_ROM);
        }
    }
    updateGL();
}

void CurveWidget::cubicInterpForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_CUBIC);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_CUBIC);
        }
    }
    updateGL();
}

void CurveWidget::horizontalInterpForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_HORIZONTAL);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_HORIZONTAL);
        }
    }
    updateGL();
}

void CurveWidget::breakTangentsForSelectedKeyFrames(){
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        SelectedKeys::iterator next = it;
        ++next;
        if(next != _selectedKeyFrames.end()){
            it->second->setInterpolation(Natron::KEYFRAME_BROKEN);
        }else{
            it->second->setInterpolationAndEvaluate(Natron::KEYFRAME_BROKEN);
        }
    }
    updateGL();
}

void CurveWidget::deleteSelectedKeyFrames(){
    _drawSelectedKeyFramesBbox = false;
    _selectedKeyFramesBbox.setBottomRight(QPointF(0,0));
    _selectedKeyFramesBbox.setTopLeft(_selectedKeyFramesBbox.bottomRight());
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
        if(_selectedKeyFrames.size() > 1){
            std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > > toRemove;
            for(SelectedKeys::iterator it = _selectedKeyFrames.begin();it!=_selectedKeyFrames.end();++it){
                toRemove.push_back(*it);
            }
            editor->removeKeyFrames(toRemove);
            
        }else{
            const std::pair<CurveGui*,boost::shared_ptr<KeyFrame> >& it = _selectedKeyFrames.front();
            editor->removeKeyFrame(it.first,it.second);
        }
        _selectedKeyFrames.clear();
        
    }else{
        while(!_selectedKeyFrames.empty()){
            const std::pair<CurveGui*,boost::shared_ptr<KeyFrame> >& it = _selectedKeyFrames.back();
            removeKeyFrame(it.first,it.second);
            _selectedKeyFrames.pop_back();
        }
    }
    
    updateGL();
}

void CurveWidget::copySelectedKeyFrames(){
    _keyFramesClipBoard.clear();
    for(SelectedKeys::iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end();++it){
        _keyFramesClipBoard.push_back(std::make_pair(it->second->getTime(),it->second->getValue()));
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
    
    for(Curves::iterator it = _curves.begin() ; it != _curves.end() ; ++it){
        if((*it)->isSelected()){
            for(U32 i = 0; i < _keyFramesClipBoard.size();++i){
                std::pair<double,Variant>& toCopy = _keyFramesClipBoard[i];
                if(!editor){
                    addKeyFrame(*it,toCopy.second,toCopy.first);
                }else{
                    editor->addKeyFrame(*it,toCopy.first,toCopy.second);
                }
            }
            updateGL();
            return;
        }
    }
    Natron::warningDialog("Curve Editor","You must select a curve first.");
}

void CurveWidget::selectAllKeyFrames(){
    _drawSelectedKeyFramesBbox = true;
    _selectedKeyFrames.clear();
    for(Curves::iterator it = _curves.begin() ; it != _curves.end() ; ++it){
        const Curve::KeyFrames& keys = (*it)->getInternalCurve()->getKeyFrames();
        for( Curve::KeyFrames::const_iterator it2 = keys.begin(); it2!=keys.end();++it2){
            _selectedKeyFrames.push_back(std::make_pair(*it,*it2));
        }
    }
    refreshSelectedKeysBbox();
    updateGL();
}

void CurveWidget::frameSelectedCurve(){
    for(Curves::iterator it = _curves.begin() ; it != _curves.end() ; ++it){
        if((*it)->isSelected()){
            std::vector<CurveGui*> curves;
            curves.push_back(*it);
            centerOn(curves);
            return;
        }
    }
    Natron::warningDialog("Curve Editor","You must select a curve first.");
}

void CurveWidget::keyFramesWithinRect(const QRectF& rect,std::vector< std::pair<CurveGui*,boost::shared_ptr<KeyFrame> > >* keys) const{
    
    double left = rect.topLeft().x();
    double right = rect.bottomRight().x();
    double bottom = rect.topLeft().y();
    double top = rect.bottomRight().y();
    for(Curves::const_iterator it = _curves.begin(); it!=_curves.end();++it){
        const Curve::KeyFrames& keyframes = (*it)->getInternalCurve()->getKeyFrames();
        for(Curve::KeyFrames::const_iterator it2 = keyframes.begin();it2 != keyframes.end();++it2){
            double y = (*it2)->getValue().value<double>();
            double x = (*it2)->getTime() ;
            if( x <= right && x >= left && y <= top && y >= bottom){
                keys->push_back(std::make_pair(*it,*it2));
            }
        }
    }
}
void CurveWidget::onTimeLineFrameChanged(SequenceTime,int){
    updateGL();
}
void CurveWidget::onTimeLineBoundariesChanged(SequenceTime,SequenceTime){
    updateGL();
}

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

#include "CurveEditor.h"

#include "Gui/ScaleSlider.h"

#include <QMenu>
#include <QMouseEvent>



static double ASPECT_RATIO = 0.1;
static double AXIS_MAX = 100000.;
static double AXIS_MIN = -100000.;

CurveEditor::CurveEditor(QWidget* parent, const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _zoomCtx()
, _dragging(false)
, _rightClickMenu(new QMenu(this))
, _clearColor(0,0,0,255)
, _baseAxisColor(118,215,90,255)
, _scaleColor(67,123,52,255)
, _textRenderer()
, _font(new QFont("Helvetica",10))

{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

CurveEditor::~CurveEditor(){
    delete _font;
}

void CurveEditor::initializeGL(){


}

void CurveEditor::centerOn(double xmin,double xmax,double ymin,double ymax){
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

void CurveEditor::resizeGL(int width,int height){
    if(height == 0)
        height = 1;
    glViewport (0, 0, width , height);
    centerOn(-10,500,-10,10);
}

void CurveEditor::paintGL(){
    double w = (double)width();
    double h = (double)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    assert(_zoomCtx._zoomFactor > 0);
    assert(_zoomCtx._zoomFactor <= 1024);
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
}


void CurveEditor::drawBaseAxis(){
    
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

void CurveEditor::drawScale(){
    QPointF btmLeft = toImgCoordinates_fast(0,height()-1);
    QPointF topRight = toImgCoordinates_fast(width()-1, 0);
    

    QFontMetrics fontM(*_font);

    /*drawing X axis*/
    double scaleWidth = width();
    double scaleYpos = btmLeft.y();
    double averageTextUnitWidth = 0.;


    averageTextUnitWidth = fontM.width(QString("-0.00000"));

    //int majorTicksCount = (scaleWidth / averageTextUnitWidth) / 2; //divide by 2 to count as much spaces between ticks as there're ticks
    int majorTicksCount = (scaleWidth / (averageTextUnitWidth+fontM.width(QString("00"))));

    double xminp,xmaxp,dist;
    double value;
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    if(majorTicksCount > 0){
        ScaleSlider::LinearScale2(btmLeft.x(), topRight.x(), majorTicksCount, acceptedDistances, &xminp, &xmaxp, &dist);
        value = xminp;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (double x = xminp; x < xmaxp;++x) {
            float alpha = 1.f;
            if(std::fmod(x,dist * 5) == 0){
                alpha = 0.8;
            }else if(std::fmod(x,dist) == 0){
                alpha = 0.6;
            }
            else if(std::fmod(x,dist / 5) == 0){
                alpha = 0.4;
            }
            else if(std::fmod(x,dist / 10) == 0){
                alpha = 0.2;
            }
            glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);

            glBegin(GL_LINES);
            glVertex2f(x, btmLeft.y());
            glVertex2f(x, topRight.y());
            glEnd();

        }
        glDisable(GL_BLEND);
    }

    for(int i = 0 ; i < majorTicksCount; ++i, value += dist) {
        QString s;
        if (dist < 1.) {
            if (std::abs(value) < dist/2.) {
                s = QString::number(0.);
            } else {
                s = QString::number(value);
            }
        } else {
            s = QString::number(std::floor(value+0.5));
        }

        renderText(value,scaleYpos , s, _scaleColor, *_font);

    }
    
    /*drawing Y axis*/
    double scaleHeight = height();
    double scaleXpos = btmLeft.x();


    majorTicksCount = (scaleHeight / fontM.height()) / 2; //divide by 2 to count as much spaces between ticks as there're ticks
    if(majorTicksCount > 0){
        ScaleSlider::LinearScale2(btmLeft.y(), topRight.y(), majorTicksCount, acceptedDistances, &xminp, &xmaxp, &dist);
        value = xminp;
    }
    for(int i = 0 ; i < majorTicksCount; ++i){
        QString s;
        if (dist < 1.) {
            if (std::abs(value) < dist/2.) {
                s = QString::number(0.);
            } else {
                s = QString::number(value);
            }
        } else {
            s = QString::number(std::floor(value+0.5));
        }

        renderText(scaleXpos,value, s, _scaleColor, *_font);


        value += dist;
    }


    //reset back the color
    glColor4f(1., 1., 1., 1.);
    
}

void CurveEditor::renderText(double x,double y,const QString& text,const QColor& color,const QFont& font){
    
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

void CurveEditor::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::RightButton){
        _rightClickMenu->exec(mapToGlobal(event->pos()));
        return;
    }
    
    _zoomCtx._oldClick = event->pos();
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _dragging = true;
    }
    QGLWidget::mousePressEvent(event);
}

void CurveEditor::mouseReleaseEvent(QMouseEvent *event){
    _dragging = false;
    QGLWidget::mouseReleaseEvent(event);
}
void CurveEditor::mouseMoveEvent(QMouseEvent *event){
    if (_dragging) {
        QPoint newClick =  event->pos();
        QPointF newClick_opengl = toImgCoordinates_fast(newClick.x(),newClick.y());
        QPointF oldClick_opengl = toImgCoordinates_fast(_zoomCtx._oldClick.x(),_zoomCtx._oldClick.y());
        float dy = (oldClick_opengl.y() - newClick_opengl.y());
        _zoomCtx._bottom += dy;
        _zoomCtx._left += (oldClick_opengl.x() - newClick_opengl.x());
        _zoomCtx._oldClick = newClick;
        updateGL();
    }
}

void CurveEditor::wheelEvent(QWheelEvent *event){
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
    QPointF zoomCenter = toImgCoordinates_fast(event->x(), event->y());
    double zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
    _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio ;
    _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    
    updateGL();

}

QPointF CurveEditor::toImgCoordinates_fast(int x,int y){
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPoint CurveEditor::toWidgetCoordinates(double x, double y){
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPoint((int)(((x - left)/(right - left))*w),(int)(((y - top)/(bottom - top))*h));
}

QSize CurveEditor::sizeHint() const{
    return QSize(1000,1000);
}

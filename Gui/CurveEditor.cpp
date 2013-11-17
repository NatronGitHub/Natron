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

static double AXIS_MAX = 100000;
static double AXIS_MIN = -100000;

CurveEditor::CurveEditor(QWidget* parent, const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _zoomCtx()
, _dragging(false)
, _rightClickMenu(new QMenu(this))
, _clearColor(0,0,0,255)
, _baseAxisColor(118,215,90,255)
, _majorAxisColor(31,50,27,255)
, _minorAxisColor(22,31,20,255)
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
    
    //initialize the box position
    _zoomCtx._left = 0. - (width()/(2.*_zoomCtx._zoomFactor));
    _zoomCtx._bottom = 0. - (height()/(2.*_zoomCtx._zoomFactor));

}

void CurveEditor::resizeGL(int width,int height){
    if(height == 0)
        height = 1;
    glViewport (0, 0, width, height);
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
    double top =  bottom +  h / (double)_zoomCtx._zoomFactor;
    double right = left +  w / (double)_zoomCtx._zoomFactor;
    if(left == right || top == bottom){
        glClearColor(_clearColor.redF(),_clearColor.greenF(),_clearColor.blueF(),_clearColor.alphaF());
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    _zoomCtx._lastOrthoLeft = left;
    _zoomCtx._lastOrthoRight = right;
    _zoomCtx._lastOrthoBottom = bottom;
    _zoomCtx._lastOrthoTop = top;
    glOrtho(left, right, bottom, top, -1, 1);
    checkGLErrors();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(_clearColor.redF(),_clearColor.greenF(),_clearColor.blueF(),_clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    
    drawBaseAxis();
    drawScale();
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
    
    double scaleWidth = topRight.x() - btmLeft.x();

    QFontMetrics fontM(*_font);
    double scaleYpos = toImgCoordinates_fast(0, height() - 1).y();
    double averageTextUnitWidth = (fontM.width(QString::number((int)btmLeft.x()))
                                + fontM.width(QString::number((int)topRight.x()))) / 2.;
    int majorTicksCount = (scaleWidth / averageTextUnitWidth) / 2; //divide by 2 to count as much spaces between ticks as there're ticks
    double unroundedTickSize = scaleWidth/(majorTicksCount-1);
    double x = ceil(log10(unroundedTickSize)-1);
    double pow10x = pow(10, x);
    double roundedTickRange = ceil(unroundedTickSize / pow10x) * pow10x;
    
    
    double value = roundedTickRange *  floor(btmLeft.x() / roundedTickRange); //minimum value to display
    while (value < roundedTickRange *  ceil(1 + topRight.x() / roundedTickRange)) {
        //drawing all values
        renderText(value,scaleYpos ,QString::number(value) , _scaleColor, *_font);
        value += roundedTickRange;
    }
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
    _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio;
    _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    
    updateGL();

}

QPoint CurveEditor::toWidgetCoordinates(int x, int y){
    double w = width() ;
    double h = height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPoint((int)(((x - left)/(right - left))*w),(int)(((y - top)/(bottom - top))*h));
}

QSize CurveEditor::sizeHint() const{
    return QSize(1000,1000);
}
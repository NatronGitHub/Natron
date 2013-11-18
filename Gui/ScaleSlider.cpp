//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "ScaleSlider.h"

#include <cassert>
#include <qlayout.h>
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QPaintEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>


#define TICK_HEIGHT 7
#define SLIDER_WIDTH 4
#define SLIDER_HEIGHT 20

ScaleSlider::ScaleSlider(double bottom, double top, double initialPos, Natron::Scale_Type type, QWidget* parent):
QGLWidget(parent,NULL)
, _zoomCtx()
, _textRenderer()
, _minimum(bottom)
, _maximum(top)
, _type(type)
, _position(initialPos)
, _XValues()
, _dragging(false)
, _font(new QFont("Times",8))
, _clearColor(50,50,50,255)
, _majorAxisColor(100,100,100,255)
, _scaleColor(100,100,100,255)
, _sliderColor(97,83,30,255)
, _initialized(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

QSize ScaleSlider::sizeHint() const{
    return QSize(150,30);
}

ScaleSlider::~ScaleSlider(){
    delete _font;
}

void ScaleSlider::initializeGL(){
    seekScalePosition(_position);
    _initialized = true;
}

void ScaleSlider::resizeGL(int width,int height){
    if(height == 0)
        height = 1;
    glViewport (0, 0, width , height);
}

void ScaleSlider::paintGL(){
    double w = (double)width();
    double h = (double)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    assert(_zoomCtx._zoomFactor > 0);
    assert(_zoomCtx._zoomFactor <= 1024);
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top = bottom +  h / (double)_zoomCtx._zoomFactor;
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
}

void ScaleSlider::drawScale(){

    //reset back the color
    glColor4f(1., 1., 1., 1.);


    QPointF btmLeft = toImgCoordinates_fast(0,height()-1);
    QPointF topRight = toImgCoordinates_fast(width()-1, 0);


    QFontMetrics fontM(*_font);

    /*drawing X axis*/
    double scaleWidth = width();
    double scaleYpos = btmLeft.y();
    double lineYpos = toImgCoordinates_fast(0,height() -1 - fontM.height()  - TICK_HEIGHT/2).y();
    double tickBottom = toImgCoordinates_fast(0,height() -1 - fontM.height() ).y();
    double tickTop = toImgCoordinates_fast(0,height() -1 - fontM.height()  - TICK_HEIGHT).y();
    double averageTextUnitWidth = 0.;

    /*draw the horizontal axis*/
    glColor4f(_scaleColor.redF(), _scaleColor.greenF(), _scaleColor.blueF(), _scaleColor.alphaF());
    glBegin(GL_LINES);
    glVertex2f(btmLeft.x(), lineYpos);
    glVertex2f(topRight.x(), lineYpos);
    glEnd();


    averageTextUnitWidth = (fontM.width(QString::number(_minimum))
                            + fontM.width(QString::number(_maximum))) / 2.;

    int majorTicksCount = (scaleWidth / averageTextUnitWidth) / 2; //divide by 2 to count as much spaces between ticks as there're ticks

    double xminp,xmaxp,dist;
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(2.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    if(_type == Natron::LINEAR_SCALE){
        ScaleSlider::LinearScale2(btmLeft.x(), topRight.x(), majorTicksCount, &xminp, &xmaxp, &dist,acceptedDistances);
    }else if(_type == Natron::LOG_SCALE){
        ScaleSlider::LogScale1(btmLeft.x(), topRight.x(), majorTicksCount, &xminp, &xmaxp, &dist,acceptedDistances);
    }

    double value = xminp;
    double prev = value;
    for(int i = 0 ; i < majorTicksCount; ++i){
        QString text = (value - prev < 1.) ?  QString::number(value,'f',1) :  QString::number((int)value);
        renderText(value ,scaleYpos , text, _scaleColor, *_font);
        /*also draw a line*/
        glColor4f(_majorAxisColor.redF(), _majorAxisColor.greenF(), _majorAxisColor.blueF(), _majorAxisColor.alphaF());
        glBegin(GL_LINES);
        glVertex2f(value, tickBottom);
        glVertex2f(value, tickTop);
        glEnd();
        //reset back the color
        glColor4f(1., 1., 1., 1.);

        prev = value;
        value += dist;
    }

    QPointF sliderBottomLeft = toImgCoordinates_fast(_position - SLIDER_WIDTH / 2,height() -1 - fontM.height()/2);
    QPointF sliderTopRight = toImgCoordinates_fast(_position + SLIDER_WIDTH / 2,height() -1 - fontM.height()/2 - SLIDER_HEIGHT);

    /*draw the slider*/

    glColor4f(_sliderColor.redF(), _sliderColor.greenF(), _sliderColor.blueF(), _sliderColor.alphaF());
    glBegin(GL_POLYGON);
    glVertex2f(sliderBottomLeft.x(),sliderBottomLeft.y());
    glVertex2f(sliderBottomLeft.x(),sliderTopRight.y());
    glVertex2f(sliderTopRight.x(),sliderTopRight.y());
    glVertex2f(sliderTopRight.x(),sliderBottomLeft.y());
    glEnd();

    /*draw a black rect around the slider for contrast*/

    glColor4f(0.,0.,0.,1.);
    glBegin(GL_LINES);
    glVertex2f(sliderBottomLeft.x(),sliderBottomLeft.y());
    glVertex2f(sliderBottomLeft.x(),sliderTopRight.y());

    glVertex2f(sliderBottomLeft.x(),sliderTopRight.y());
    glVertex2f(sliderTopRight.x(),sliderTopRight.y());

    glVertex2f(sliderTopRight.x(),sliderTopRight.y());
    glVertex2f(sliderTopRight.x(),sliderBottomLeft.y());

    glVertex2f(sliderTopRight.x(),sliderBottomLeft.y());
    glVertex2f(sliderBottomLeft.x(),sliderBottomLeft.y());
    glEnd();

    //reset back the color
    glColor4f(1., 1., 1., 1.);
}

void ScaleSlider::renderText(double x,double y,const QString& text,const QColor& color,const QFont& font){

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
    QFontMetrics m(*_font);
    _textRenderer.renderText(pos.x() - m.width(text)/2,h-pos.y(),text,color,font);
    checkGLErrors();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(_zoomCtx._lastOrthoLeft,_zoomCtx._lastOrthoRight,_zoomCtx._lastOrthoBottom,_zoomCtx._lastOrthoTop,-1,1);
    glMatrixMode(GL_MODELVIEW);

}



void ScaleSlider::mousePressEvent(QMouseEvent *event){

    _zoomCtx._oldClick = event->pos();

    QGLWidget::mousePressEvent(event);
}


void ScaleSlider::mouseMoveEvent(QMouseEvent *event){
    QPoint newClick =  event->pos();
    QPointF newClick_opengl = toImgCoordinates_fast(newClick.x(),newClick.y());

    seekScalePosition(newClick_opengl.x());

}


void ScaleSlider::seekScalePosition(double v){
    _position = toWidgetCoordinates(v,0).x();
    double padding = (_maximum - _minimum) / 10.;
    double displayedRange = _maximum - _minimum + 2*padding;
    double zoomFactor = width() /displayedRange;
    zoomFactor = (zoomFactor > 0.06) ? (zoomFactor-0.05) : std::max(zoomFactor,0.01);
    assert(zoomFactor>=0.01 && zoomFactor <= 1024);
    _zoomCtx.setZoomFactor(zoomFactor);
    _zoomCtx._left = _minimum - padding ;
    _zoomCtx._bottom = 0;
    if(_initialized)
        updateGL();

}

QPointF ScaleSlider::toImgCoordinates_fast(int x,int y){
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPoint ScaleSlider::toWidgetCoordinates(double x, double y){
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPoint((int)(((x - left)/(right - left))*w),(int)(((y - top)/(bottom - top))*h));
}


// See http://lists.gnu.org/archive/html/octave-bug-tracker/2011-09/pdfJd5VVqNUGE.pdf

void ScaleSlider::LinearScale1(double xmin, double xmax, int n, double* xminp, double* xmaxp, double *dist){
    static double vint[4] = { 1.f,2.f,5.f,10.f };
    static double sqr[3] = { 1.414214,3.162278,7.071068}; //sqrt(2), sqrt(10), sqrt(50)
    
    if(xmax <= xmin || n == 0) //improper range
        return;
    
    double del =  2e-5f;
    double a = (xmax - xmin) / (double)n;
    double al = std::log10(a);
    int nal = al;
    if(a < 1.){
        --nal;
    }
    a /= std::pow(10.,nal);
    int i = 0;
    while(a > sqr[i] && i < 3){
        ++i;
    }
    *dist =  vint[i] * std::pow(10.,nal);
    double fm1 = xmin / *dist;
    int m1 = fm1;
    if(fm1 < 0.)
        --m1;
    double r_1 = m1 + 1. - fm1;
    if(std::abs(r_1) <  del){
        ++m1;
    }
    *xminp = *dist * m1;
    double fm2 = xmax / *dist;
    int m2 = fm2 + 1.;
    if(fm2 <  -1.){
        --m2;
    }
    r_1 = fm2 + 1. - m2;
    if(std::abs(r_1) < del){
        --m2;
    }
    *xmaxp = *dist * m2;
    if(*xminp > xmin){
        *xminp = xmin;
    }
    if(*xmaxp < xmax){
        *xmaxp = xmax;
    }
}

void ScaleSlider::LinearScale2(double xmin, double xmax, int n, double* xminp, double* xmaxp, double *dist,
                               const std::vector<double> &acceptedDistances){
   // static double vint[3] = {1., /*2.,*/5.,10./*,20.*/ };
    

    if(xmax <= xmin || n == 0) //improper range
        return;
    
    double del =  0.00002;
    double a = (xmax - xmin) / (double)n;
    double al = std::log10(a);
    int nal = al;
    if(a < 1.){
        --nal;
    }
    a /= std::pow(10,nal);
    int i = 0;
    int np  = 0;
    do{
        while(a >= (acceptedDistances[i]+del) && i < (int)acceptedDistances.size()){
            ++i;
        }
        *dist =  acceptedDistances[i] *  std::pow(10,nal);
        double fm1 = xmin / *dist;
        int m1 = fm1;
        if(fm1 < 0.)
            --m1;
        double r_1 = m1 + 1. - fm1;
        if(std::abs(r_1) <  del){
            ++m1;
        }
        *xminp = *dist * (double)m1;
        double fm2 = xmax / *dist;
        int m2 = fm2 + 1.;
        if(fm2 <  -1.){
            --m2;
        }
        r_1 = fm2 + 1. - m2;
        if(std::abs(r_1) < del){
            --m2;
        }
        *xmaxp = *dist * (double)m2;
        np = (m2 - m1);
        ++i;
    }while(np > n);
    int nx = (n - np) / 2;
    *xminp -= (double)nx * *dist;
    *xmaxp = *xminp + (double)n * *dist;
    
    if (*xminp > xmin) {
        *xminp = xmin;
    }
    if (*xmaxp < xmax) {
        *xmaxp = xmax;
    }
}

void ScaleSlider::LogScale1(double xmin, double xmax, int n, double* xminp, double* xmaxp, double *dist,
                            const std::vector<double> &acceptedDistances){
    //static double vint[11] = { 10.f,9.f,8.f,7.f,6.f,5.f,4.f,3.f,2.f,1.f,.5f };
    
    if(xmax <= xmin || n <= 1 || xmin <= 0.) //improper range
        return;
    
    double del =  2e-5f;
    double xminl = std::log10(xmin);
    double xmaxl = std::log10(xmax);
    double a = (xmaxl - xminl) / (double)n;
    double al = std::log10(a);
    int nal = al;
    if(a < 1.f){
        --nal;
    }
    
    a /= std::pow(10,nal);
    int i = 0;
    while(a >= 10. / acceptedDistances[i] + del && i < (int)acceptedDistances.size()){
        ++i;
    }
    int i_1 = nal + 1;
    int np = 0;
    double distl;
    do{
        distl = std::pow(10,i_1) / acceptedDistances[i];
        double fm1 = xminl / distl;
        int m1 = fm1;
        if(fm1 < 0.){
            --m1;
        }
        double r_1 = (double)m1 + 1. - fm1;
        if(std::abs(r_1) < del){
            ++m1;
        }
        *xminp = distl * (double)m1;
        double fm2 = xmaxl / distl;
        int m2 = fm2 + 1.;
        if (fm2 < -1.) {
            --m2;
        }
        r_1 = fm2 + 1. - (double)m2;
        if(std::abs(r_1) < del){
            --m2;
        }
        *xmaxp = distl * (double)m2;
        
        np = m2 - m1;
        ++i;
    }while(np > n);
    
    int nx = (n - np) / 2;
    *xminp -= (double)nx * distl;
    *xmaxp = *xminp + (double)n * distl;
    
    *dist = std::pow(10.,distl);
    *xminp = std::pow(10,*xminp);
    *xmaxp = std::pow(10,*xmaxp);
    
    if(*xminp > xmin){
        *xminp = xmin;
    }
    if(*xmaxp < xmax){
        *xmaxp = xmax;
    }
}

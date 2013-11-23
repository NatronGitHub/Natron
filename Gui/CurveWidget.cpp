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

#include "Engine/Knob.h"
#include "Gui/ScaleSlider.h"

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
// (in widget pixels)

static double ASPECT_RATIO = 0.1;
static double AXIS_MAX = 100000.;
static double AXIS_MIN = -100000.;

CurveWidget::CurveWidget(QWidget* parent, const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _zoomCtx()
, _dragging(false)
, _rightClickMenu(new QMenu(this))
, _clearColor(0,0,0,255)
, _baseAxisColor(118,215,90,255)
, _scaleColor(67,123,52,255)
, _selectedCurveColor(255,255,89,255)
, _textRenderer()
, _font(new QFont("Helvetica",10))
, _curves()
, _selectedKeyFrames()
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

CurveWidget::~CurveWidget(){
    delete _font;
}

void CurveWidget::initializeGL(){


}

void CurveWidget::addCurve(boost::shared_ptr<CurveGui> curve){
    _curves.push_back(curve);
}

void CurveWidget::removeCurve(boost::shared_ptr<CurveGui> curve){
    for(std::list<boost::shared_ptr<CurveGui> >::iterator it = _curves.begin();it!=_curves.end();++it){
        if((*it) == curve){
            _curves.erase(it);
            break;
        }
    }
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

void CurveWidget::paintGL(){
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

    drawCurves();
}

void CurveWidget::drawCurves(){
    //now draw each curve
    for(std::list<boost::shared_ptr<CurveGui> >::const_iterator it = _curves.begin();it!=_curves.end();++it){
        if((*it)->isVisible()){
            const QColor& curveColor =  (*it)->isSelected() ?  _selectedCurveColor :  (*it)->getColor();
            const QColor& nameColor =  (*it)->getColor();

            glColor4f(curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF());
            
            (*it)->beginRecordBoundingBox();
            for(int i = 0; i < width();++i){
                double x = toImgCoordinates_fast(i,0).x();
                double y = (*it)->evaluate(x);
                if(i == 10){ // draw the name of the curve on the left of the widget
                    glColor4f(1., 1., 1., 1.);
                    renderText(x,y,(*it)->getName(),nameColor,*_font);
                    glColor4f(curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF());
                }
                glPointSize((*it)->getThickness());
                glBegin(GL_POINTS);
                glVertex2f(x,y);
                glEnd();
            }
            (*it)->endRecordBoundingBox();

            // draw the keyframes
            const CurvePath::KeyFrames& keyframes = (*it)->getInternalCurve().getKeyFrames();

            glPointSize(7);

            for(CurvePath::KeyFrames::const_iterator k = keyframes.begin();k!=keyframes.end();++k){
                glColor4f(nameColor.redF(), nameColor.greenF(), nameColor.blueF(), nameColor.alphaF());
                KeyFrame key = (*k);
                //if the key is selected change its color to white
                for(std::list< KeyFrame >::const_iterator it2 = _selectedKeyFrames.begin();
                    it2 != _selectedKeyFrames.end();++it2){
                    if((*it2) == key){
                         glColor4f(1.f,1.f,1.f,1.f);
                         break;
                    }
                }

                glBegin(GL_POINTS);
                glVertex2f(key.getTime(),key.getValue().toDouble());
                glEnd();

            }

        }
    }
    glPointSize(1.f);

    //reset back the color
    glColor4f(1., 1., 1., 1.);

}

void CurveGui::beginRecordBoundingBox() const{
    _internalCurve.beginRecordBoundingBox();
}

void CurveGui::endRecordBoundingBox() const{
    _internalCurve.endRecordBoundingBox();
}

void CurveWidget::drawBaseAxis(){
    
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

static inline
double ticks_alpha(double min, double max, double val)
{
    assert(val > 0. && min > 0. && max > 0. && max > min);
    const double alpha = sqrt((val-min)/(max-min));
    return std::max(0.,std::min(alpha,1.));
}

void CurveWidget::drawScale(){
    QPointF btmLeft = toImgCoordinates_fast(0,height()-1);
    QPointF topRight = toImgCoordinates_fast(width()-1, 0);
    

    QFontMetrics fontM(*_font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    int jmax;
    double xminp,xmaxp,dist;
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    
    { /*drawing X axis*/
        double rangePixel = width(); // AXIS-SPECIFIC
        double range_min = btmLeft.x(); // AXIS-SPECIFIC
        double range_max = topRight.x(); // AXIS-SPECIFIC
        double minTickSizePixel = fontM.width(QString("-0.00000")) + fontM.width(QString("00")); // AXIS-SPECIFIC

        double range = range_max - range_min;
        int majorTicksCount = (rangePixel / minTickSizePixel) + 2;
        double minTickSize = range * minTickSizePixel/rangePixel;
        ScaleSlider::LinearScale2(range_min - minTickSize, range_max + minTickSize, majorTicksCount, acceptedDistances, &xminp, &xmaxp, &dist);

        int m1 = floor(xminp/dist + 0.5);
        int m2 = floor(xmaxp/dist + 0.5);
        double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        double largestTickSize = range * largestTickSizePixel / rangePixel;
        assert(smallestTickSize > 0);
        assert(largestTickSize > 0);
        {
            double log10dist = std::log10(dist);
            if (std::abs(log10dist - std::floor(log10dist+0.5)) < 0.001) {
                // dist is a power of 10
                jmax = 10;
            } else {
                jmax = 50;
            }
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for(int i = m1 ; i <= m2; ++i) {
            double value = i * dist;

            for (int j=0; j < jmax; ++j) {
                int tickCount = 1;

                if (i == 0 && j == 0) {
                    tickCount = 10000; // special case for the axes
                } else if (j == 0) {
                    if (i % 100 == 0) {
                        tickCount = 100*jmax;
                    } else if (i % 50 == 0) {
                        tickCount = 50*jmax;
                    } else if (i % 10 == 0) {
                        tickCount = 10*jmax;
                    } else if (i % 5 == 0) {
                        tickCount = 5*jmax;
                    } else {
                        tickCount = 1*jmax;
                    }
                } else if (j % 25 == 0) {
                    tickCount = 25;
                } else if (j % 10 == 0) {
                    tickCount = 10;
                } else if (j % 5 == 0) {
                    if (jmax == 10 && (i*jmax+j) % 25 == 0) {
                        tickCount = 25;
                    } else {
                        tickCount = 5;
                    }
                }
                const double alpha = ticks_alpha(smallestTickSize,largestTickSize, tickCount*dist/(double)jmax);

                glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);

                glBegin(GL_LINES);
                double u = value + j*dist/(double)jmax;
                glVertex2f(u, btmLeft.y()); // AXIS-SPECIFIC
                glVertex2f(u, topRight.y()); // AXIS-SPECIFIC
                glEnd();
            }

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

            renderText(value, btmLeft.y(), s, _scaleColor, *_font); // AXIS-SPECIFIC
        }
        glDisable(GL_BLEND);
    }
    { /*drawing Y axis*/
        double rangePixel = height(); // AXIS-SPECIFIC
        double range_min = btmLeft.y(); // AXIS-SPECIFIC
        double range_max = topRight.y(); // AXIS-SPECIFIC
        double minTickSizePixel = fontM.height() * 2; // AXIS-SPECIFIC

        double range = range_max - range_min;
        int majorTicksCount = (rangePixel / minTickSizePixel) + 2;
        double minTickSize = range * minTickSizePixel / rangePixel;
        ScaleSlider::LinearScale2(range_min - minTickSize, range_max + minTickSize, majorTicksCount, acceptedDistances, &xminp, &xmaxp, &dist);

        int m1 = floor(xminp/dist + 0.5);
        int m2 = floor(xmaxp/dist + 0.5);
        double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        double largestTickSize = range * largestTickSizePixel / rangePixel;
        assert(smallestTickSize > 0);
        assert(largestTickSize > 0);
        {
            double log10dist = std::log10(dist);
            if (std::abs(log10dist - std::floor(log10dist+0.5)) < 0.001) {
                // dist is a power of 10
                jmax = 10;
            } else {
                jmax = 50;
            }
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for(int i = m1 ; i <= m2; ++i) {
            double value = i * dist;

            for (int j=0; j < jmax; ++j) {
                int tickCount = 1;

                if (i == 0 && j == 0) {
                    tickCount = 10000; // special case for the axes
                } else if (j == 0) {
                    if (i % 100 == 0) {
                        tickCount = 100*jmax;
                    } else if (i % 50 == 0) {
                        tickCount = 50*jmax;
                    } else if (i % 10 == 0) {
                        tickCount = 10*jmax;
                    } else if (i % 5 == 0) {
                        tickCount = 5*jmax;
                    } else {
                        tickCount = 1*jmax;
                    }
                } else if (j % 25 == 0) {
                    tickCount = 25;
                } else if (j % 10 == 0) {
                    tickCount = 10;
                } else if (j % 5 == 0) {
                    if (jmax == 10 && (i*jmax+j) % 25 == 0) {
                        tickCount = 25;
                    } else {
                        tickCount = 5;
                    }
                }
                const double alpha = ticks_alpha(smallestTickSize,largestTickSize, tickCount*dist/(double)jmax);
                glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);
                
                glBegin(GL_LINES);
                double u = value + j*dist/(double)jmax;
                glVertex2f(btmLeft.x(), u); // AXIS-SPECIFIC
                glVertex2f(topRight.x(), u); // AXIS-SPECIFIC
                glEnd();
                
            }
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

            renderText(btmLeft.x(), value, s, _scaleColor, *_font); // AXIS-SPECIFIC
        }
        glDisable(GL_BLEND);
    }
    //reset back the color
    glColor4f(1., 1., 1., 1.);
    
}

void CurveWidget::renderText(double x,double y,const QString& text,const QColor& color,const QFont& font){
    
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
    QPointF openGL_pos = toImgCoordinates_fast(pt.x(),pt.y());
    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
        double y = (*it)->evaluate(openGL_pos.x());
        int yWidget = toWidgetCoordinates(0,y).y();
        if(std::abs(pt.y() - yWidget) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE){
            return it;
        }
    }
    return _curves.end();
}

void CurveWidget::selectCurve(boost::shared_ptr<CurveGui> curve){
    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
        (*it)->setSelected(false);
    }
    curve->setSelected(true);
}

void CurveWidget::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::RightButton){
        _rightClickMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    CurveWidget::Curves::const_iterator foundCurveNearby = isNearbyCurve(event->pos());
    if(foundCurveNearby != _curves.end()){
        selectCurve(*foundCurveNearby);
    }

    _zoomCtx._oldClick = event->pos();
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _dragging = true;
    }
    QGLWidget::mousePressEvent(event);
    updateGL();
}

void CurveWidget::mouseReleaseEvent(QMouseEvent *event){
    _dragging = false;
    QGLWidget::mouseReleaseEvent(event);
}
void CurveWidget::mouseMoveEvent(QMouseEvent *event){
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
    QPointF zoomCenter = toImgCoordinates_fast(event->x(), event->y());
    double zoomRatio =   _zoomCtx._zoomFactor / newZoomFactor;
    _zoomCtx._left = zoomCenter.x() - (zoomCenter.x() - _zoomCtx._left)*zoomRatio ;
    _zoomCtx._bottom = zoomCenter.y() - (zoomCenter.y() - _zoomCtx._bottom)*zoomRatio;
    
    _zoomCtx._zoomFactor = newZoomFactor;
    
    updateGL();

}

QPointF CurveWidget::toImgCoordinates_fast(int x,int y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPoint CurveWidget::toWidgetCoordinates(double x, double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _zoomCtx._bottom;
    double left = _zoomCtx._left;
    double top =  bottom +  h / _zoomCtx._zoomFactor * ASPECT_RATIO;
    double right = left +  w / _zoomCtx._zoomFactor;
    return QPoint((int)(((x - left)/(right - left))*w),(int)(((y - top)/(bottom - top))*h));
}

QSize CurveWidget::sizeHint() const{
    return QSize(1000,1000);
}

double CurveGui::evaluate(double x) const{
    return _internalCurve.getValueAt(x).toDouble();
}



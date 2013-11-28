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
#include "Gui/ticks.h"

#define CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE 5 //maximum distance from a curve that accepts a mouse click
// (in widget pixels)

static double ASPECT_RATIO = 0.1;
static double AXIS_MAX = 100000.;
static double AXIS_MIN = -100000.;


CurveGui::CurveGui(const CurveWidget *curveWidget,
                   boost::shared_ptr<CurvePath> curve,
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

    if(curve->getControlPointsCount() > 1){
        _visible = true;
    }

    QObject::connect(this,SIGNAL(curveChanged()),curveWidget,SLOT(updateGL()));

}

CurveGui::~CurveGui(){

}

void CurveGui::drawCurve(){
    if(!_visible)
        return;

    assert(QGLContext::currentContext() == _curveWidget->context());
    beginRecordBoundingBox();

    int w = _curveWidget->width();
    float* vertices = new float[w * 2];
    int vertIndex = 0;
    for(int i = 0 ; i < w;++i,vertIndex+=2){
        double x = _curveWidget->toScaleCoordinates(i,0).x();
        double y = evaluate(x);
        vertices[vertIndex] = (float)x;
        vertices[vertIndex+1] = (float)y;
    }

    endRecordBoundingBox();

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
    for(int i = 0; i < w*2;i+=2){
        glVertex2f(vertices[i],vertices[i+1]);
    }
    glEnd();


    glDisable(GL_LINE_SMOOTH);
    checkGLErrors();
    delete [] vertices;

    glLineWidth(1.);


    //render the name of the curve
    glColor4f(1.f, 1.f, 1.f, 1.f);
    double textX = _curveWidget->toScaleCoordinates(15,0).x();
    double textY = evaluate(textX);
    _curveWidget->renderText(textX,textY,_name,_color,_curveWidget->getFont());
    glColor4f(curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF());


    //draw keyframes
    const CurvePath::KeyFrames& keyframes = _internalCurve->getKeyFrames();
    glPointSize(7.f);
    glEnable(GL_POINT_SMOOTH);

    glBegin(GL_POINTS);
    for(CurvePath::KeyFrames::const_iterator k = keyframes.begin();k!=keyframes.end();++k){
        glColor4f(_color.redF(), _color.greenF(), _color.blueF(), _color.alphaF());
        KeyFrame* key = (*k);
        //if the key is selected change its color to white
        const std::list< KeyFrame* >& selectedKeyFrames = _curveWidget->getSelectedKeyFrames();
        for(std::list< KeyFrame* >::const_iterator it2 = selectedKeyFrames.begin();
            it2 != selectedKeyFrames.end();++it2){
            if((*it2) == key){
                glColor4f(1.f,1.f,1.f,1.f);
                break;
            }
        }
        glVertex2f(key->getTime(),key->getValue().toDouble());

    }
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
    glPopAttrib();
    glPointSize(1.f);
    //reset back the color
    glColor4f(1.f, 1.f, 1.f, 1.f);


}

double CurveGui::evaluate(double x) const{
    return _internalCurve->getValueAt(x).toDouble();
}

void CurveGui::setVisible(bool visible) { _visible = visible; emit curveChanged(); }

CurveWidget::CurveWidget(QWidget* parent, const QGLWidget* shareWidget)
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
    , _font(new QFont("Helvetica",10))
    , _curves()
    , _selectedKeyFrames()
    , _hasOpenGLVAOSupport(true)
    , _mouseDragOrientation()
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);
    _nextCurveAddedColor.setHsv(200,255,255);
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

CurveGui* CurveWidget::createCurve(boost::shared_ptr<CurvePath> curve,const QString& name){
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
            const CurvePath::KeyFrames& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (CurvePath::KeyFrames::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                std::list<KeyFrame*>::iterator foundSelected = std::find(_selectedKeyFrames.begin(), _selectedKeyFrames.end(), *it2);
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

    drawCurves();
}

void CurveWidget::drawCurves()
{
    assert(QGLContext::currentContext() == context());
    //now draw each curve
    for(std::list<CurveGui*>::const_iterator it = _curves.begin();it!=_curves.end();++it){
        (*it)->drawCurve();
    }
}

void CurveGui::beginRecordBoundingBox() const{
    _internalCurve->beginRecordBoundingBox();
}

void CurveGui::endRecordBoundingBox() const{
    _internalCurve->endRecordBoundingBox();
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
#if 1 // use new version of graph paper drawing
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
#else // use old version of graph paper drawing
        const double minTickSizePixel = axis == 0 ? fontM.width(QString("-0.00000")) + fontM.width(QString("00")) : fontM.height() * 2; // AXIS-SPECIFIC
        const int majorTicksCount = (rangePixel / minTickSizePixel) + 2;
        const double minTickSize = range * minTickSizePixel/rangePixel;
        double xminp,xmaxp,dist;
        ScaleSlider::LinearScale2(range_min - minTickSize, range_max + minTickSize, majorTicksCount, acceptedDistances, &xminp, &xmaxp, &dist);

        const int m1 = floor(xminp/dist + 0.5);
        const int m2 = floor(xmaxp/dist + 0.5);
        const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        const double largestTickSize = range * largestTickSizePixel / rangePixel;
        assert(smallestTickSize > 0);
        assert(largestTickSize > 0);
        int jmax;
        {
            const double log10dist = std::log10(dist);
            if (std::abs(log10dist - std::floor(log10dist+0.5)) < 0.001) {
                // dist is a power of 10
                jmax = 10;
            } else {
                jmax = 50;
            }
        }
        std::cout << " m1=" << m1 << " m2=" << m2 << std::endl;

        for(int i = m1 ; i <= m2; ++i) {
            const double value = i * dist;

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
                const double u = value + j*dist/(double)jmax;
                if (axis == 0) {
                    glVertex2f(u, btmLeft.y()); // AXIS-SPECIFIC
                    glVertex2f(u, topRight.y()); // AXIS-SPECIFIC
                } else {
                    glVertex2f(btmLeft.x(), u); // AXIS-SPECIFIC
                    glVertex2f(topRight.x(), u); // AXIS-SPECIFIC
                }
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

            if (axis == 0) {
                renderText(value, btmLeft.y(), s, _scaleColor, *_font); // AXIS-SPECIFIC
            } else {
                renderText(btmLeft.x(), value, s, _scaleColor, *_font); // AXIS-SPECIFIC
            }
        }
#endif
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
            int yWidget = toWidgetCoordinates(0,y).y();
            if(std::abs(pt.y() - yWidget) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE){
                return it;
            }
        }
    }
    return _curves.end();
}

KeyFrame* CurveWidget::isNearbyKeyFrame(const QPoint& pt) const{
    for(Curves::const_iterator it = _curves.begin();it!=_curves.end();++it){
        if((*it)->isVisible()){
            const CurvePath::KeyFrames& keyFrames = (*it)->getInternalCurve()->getKeyFrames();
            for (CurvePath::KeyFrames::const_iterator it2 = keyFrames.begin(); it2 != keyFrames.end(); ++it2) {
                QPoint keyFramewidgetPos = toWidgetCoordinates((*it2)->getTime(), (*it2)->getValue().toDouble());
                if((std::abs(pt.y() - keyFramewidgetPos.y()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE) &&
                   (std::abs(pt.x() - keyFramewidgetPos.x()) < CLICK_DISTANCE_FROM_CURVE_ACCEPTANCE)){
                    return (*it2);
                }
            }
        }
    }
    return NULL;
}

void CurveWidget::selectCurve(CurveGui* curve){
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
    _selectedKeyFrames.clear();
    KeyFrame* selectedKey = isNearbyKeyFrame(event->pos());
    if(selectedKey){
        _state = DRAGGING_KEYS;
        setCursor(QCursor(Qt::CrossCursor));
        _selectedKeyFrames.push_back(selectedKey);
    }

    _zoomCtx._oldClick = event->pos();
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _state = DRAGGING_VIEW;
    }
    QGLWidget::mousePressEvent(event);
    updateGL();
}

void CurveWidget::mouseReleaseEvent(QMouseEvent *event){
    _state = NONE;
    QGLWidget::mouseReleaseEvent(event);
}
void CurveWidget::mouseMoveEvent(QMouseEvent *event){
    KeyFrame* selectedKey = isNearbyKeyFrame(event->pos());
    if(selectedKey){
        setCursor(QCursor(Qt::CrossCursor));
    }else{
        setCursor(QCursor(Qt::ArrowCursor));
    }

    
    
    if(std::abs(event->x() - _zoomCtx._oldClick.x()) > std::abs(event->y() - _zoomCtx._oldClick.y())){
        _mouseDragOrientation.setX(1);
        _mouseDragOrientation.setY(0);
    }else{
        _mouseDragOrientation.setX(0);
        _mouseDragOrientation.setY(1);
    }
    QPoint newClick =  event->pos();
    QPointF newClick_opengl = toScaleCoordinates(newClick.x(),newClick.y());
    QPointF oldClick_opengl = toScaleCoordinates(_zoomCtx._oldClick.x(),_zoomCtx._oldClick.y());
    
       
    _zoomCtx._oldClick = newClick;
    if (_state == DRAGGING_VIEW) {
        
        float dy = (oldClick_opengl.y() - newClick_opengl.y());
        _zoomCtx._bottom += dy;
        _zoomCtx._left += (oldClick_opengl.x() - newClick_opengl.x());
    }else if(_state == DRAGGING_KEYS){
        
        QPointF translation = (newClick_opengl - oldClick_opengl);
        translation.rx() *= _mouseDragOrientation.x();
        translation.ry() *= _mouseDragOrientation.y();
        
        for (std::list<KeyFrame*>::const_iterator it = _selectedKeyFrames.begin(); it != _selectedKeyFrames.end(); ++it) {
            (*it)->setTime((*it)->getTime() + translation.x());
            (*it)->setValue(Variant((*it)->getValue().toDouble() + translation.y()));
        }
    }
    updateGL();
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

QPointF CurveWidget::toScaleCoordinates(int x,int y) const {
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




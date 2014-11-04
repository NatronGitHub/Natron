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

#include "ScaleSliderQWidget.h"

#include <vector>
#include <cmath> // for std::pow()
#include <cassert>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QPaintEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QStyleOption>

#include "Gui/ticks.h"

#define TICK_HEIGHT 7
#define SLIDER_WIDTH 4
#define SLIDER_HEIGHT 15

class ZoomHelper
{
public:
    ZoomHelper()
    : _zoomLeft(0.)
    , _zoomBottom(0.)
    , _zoomFactor(1.)
    , _zoomPAR(1.)
    , _screenWidth(0)
    , _screenHeight(0)
    {
    }
    
    double left() const
    {
        return _zoomLeft;
    }
    
    double right() const
    {
        return _zoomLeft + _screenWidth / (_zoomFactor * _zoomPAR);
    }
    
    double bottom() const
    {
        return _zoomBottom;
    }
    
    double top() const
    {
        return _zoomBottom + _screenHeight / _zoomFactor;
    }

    /// width in screen pixels of the zoomed area
    double screenWidth() const
    {
        return _screenWidth;
    }
    
    /// height in screen pixels of the zoomed area
    double screenHeight() const
    {
        return _screenHeight;
    }
    
    
    // only zoom the x axis: changes the PAR and the Left but not the zoomFactor or the bottom
    void zoomx(double centerX,
               double /*centerY*/,
               double scale)
    {
        _zoomLeft = centerX - ( centerX - left() ) / scale;
        _zoomPAR *= scale;
    }


    
    // fill the area (xmin-xmax,ymin-ymax) in the zoom window, modifying the PAR
    void fill(double xmin,
              double xmax,
              double ymin,
              double ymax)
    {
        double width = xmax - xmin;
        double height = ymax - ymin;
        
        _zoomLeft = xmin;
        _zoomBottom = ymin;
        _zoomFactor = screenHeight() / height;
        _zoomPAR = (screenWidth() * height) / (screenHeight() * width);
    }
    
    void setScreenSize(double screenWidth,
                       double screenHeight)
    {
        _screenWidth = screenWidth;
        _screenHeight = screenHeight;
    }
    
    /**
     *@brief Computes the image coordinates of the point passed in parameter.
     * This is a fast in-line method much faster than toZoomCoordinates_slow().
     * This function actually does the unprojection to retrieve the position.
     *@param x[in] The x coordinate of the point in viewport coordinates.
     *@param y[in] The y coordinates of the point in viewport coordinates.
     *@returns Returns the image coordinates mapped equivalent of (x,y).
     **/
    QPointF toZoomCoordinates(double widgetX,
                              double widgetY) const
    {
        return QPointF( ( ( ( right() - left() ) * widgetX ) / screenWidth() ) + left(),
                       ( ( ( bottom() - top() ) * widgetY ) / screenHeight() ) + top() );
    }
    
    /**
     *@brief Computes the viewport coordinates of the point passed in parameter.
     * This function actually does the projection to retrieve the position;
     *@param x[in] The x coordinate of the point in image coordinates.
     *@param y[in] The y coordinates of the point in image coordinates.
     *@returns Returns the viewport coordinates mapped equivalent of (x,y).
     **/
    QPointF toWidgetCoordinates(double zoomX,
                                double zoomY) const
    {
        return QPointF( ( ( zoomX - left() ) / ( right() - left() ) ) * screenWidth(),
                       ( ( zoomY - top() ) / ( bottom() - top() ) ) * screenHeight() );
    }
    
private:
    double _zoomLeft; /// the left edge of the orthographic projection
    double _zoomBottom; /// the bottom edge of orthographic projection
    double _zoomFactor; /// the zoom factor applied to the current image
    double _zoomPAR; /// the pixel aspect ration; a pixel from the image data occupies a size (zoomFactor*zoomPar,zoomFactor)
    double _screenWidth; /// window width in screen pixels
    double _screenHeight; /// window height in screen pixels
};


struct ScaleSliderQWidgetPrivate
{
    
    ZoomHelper zoomCtx;
    QPointF oldClick;
    double minimum,maximum;
    Natron::ScaleTypeEnum type;
    double value;
    bool dragging;
    QFont* font;
    QColor textColor;
    QColor scaleColor;
    QColor sliderColor;
    bool initialized;
    bool mustInitializeSliderPosition;
    bool readOnly;
    bool ctrlDown;
    bool shiftDown;
    double currentZoom;
    
    ScaleSliderQWidgetPrivate(double min,
                              double max,
                              double initialPos,
                              Natron::ScaleTypeEnum type)
    : zoomCtx()
    , oldClick()
    , minimum(min)
    , maximum(max)
    , type(type)
    , value(initialPos)
    , dragging(false)
    , font(new QFont(NATRON_FONT_ALT, NATRON_FONT_SIZE_8))
    , textColor(200,200,200,255)
    , scaleColor(100,100,100,255)
    , sliderColor(97,83,30,255)
    , initialized(false)
    , mustInitializeSliderPosition(true)
    , readOnly(false)
    , ctrlDown(false)
    , shiftDown(false)
    , currentZoom(1.)
    {
        
    }
};

ScaleSliderQWidget::ScaleSliderQWidget(double min,
                                       double max,
                                       double initialPos,
                                       Natron::ScaleTypeEnum type,
                                       QWidget* parent)
    : QWidget(parent)
    , _imp(new ScaleSliderQWidgetPrivate(min,max,initialPos,type))
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QSize sizeh = sizeHint();
    _imp->zoomCtx.setScreenSize(sizeh.width(), sizeh.height());
    setFocusPolicy(Qt::ClickFocus);
}

QSize
ScaleSliderQWidget::sizeHint() const
{
    return QSize(150,20);
}

ScaleSliderQWidget::~ScaleSliderQWidget()
{
    delete _imp->font;
}

Natron::ScaleTypeEnum
ScaleSliderQWidget::type() const
{
    return _imp->type;
}

double
ScaleSliderQWidget::minimum() const
{
    return _imp->minimum;
}

double
ScaleSliderQWidget::maximum() const
{
    return _imp->maximum;
}

double
ScaleSliderQWidget::getPosition() const
{
    return _imp->value;
}

bool
ScaleSliderQWidget::isReadOnly() const
{
    return _imp->readOnly;
}


void
ScaleSliderQWidget::mousePressEvent(QMouseEvent* e)
{
    if (!_imp->readOnly) {
        QPoint newClick =  e->pos();

        _imp->oldClick = newClick;
        QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( newClick.x(),newClick.y() );

        seekInternal( newClick_opengl.x() );
    }
    QWidget::mousePressEvent(e);
}

void
ScaleSliderQWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!_imp->readOnly) {
        QPoint newClick =  e->pos();
        QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( newClick.x(),newClick.y() );

        seekInternal( newClick_opengl.x() );
    }
}

void
ScaleSliderQWidget::mouseReleaseEvent(QMouseEvent* e)
{
    emit editingFinished();
    QWidget::mouseReleaseEvent(e);
}

void
ScaleSliderQWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        _imp->ctrlDown = true;
        double scale = _imp->shiftDown ? 100. : 10.;
        _imp->currentZoom = scale;
        _imp->zoomCtx.zoomx(_imp->value, 0, scale);
        update();
    } else if (e->key() == Qt::Key_Shift) {
        _imp->shiftDown = true;
        if (_imp->ctrlDown) {
            _imp->zoomCtx.zoomx(_imp->value, 0, 10.);
            _imp->currentZoom = 100.;
        }
        update();
    }
    QWidget::keyPressEvent(e);
}

void
ScaleSliderQWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        _imp->ctrlDown = false;
        _imp->zoomCtx.zoomx(_imp->value, 0, 1. / _imp->currentZoom);
        _imp->currentZoom = 1.;
        centerOn(_imp->minimum, _imp->maximum);
        return;
    } else if (e->key() == Qt::Key_Shift) {
        _imp->shiftDown = false;
        if (_imp->ctrlDown) {
            _imp->zoomCtx.zoomx(_imp->value, 0, 1. / 10.);
            _imp->currentZoom = 10.;
        } else {
            _imp->zoomCtx.zoomx(_imp->value, 0, 1. / _imp->currentZoom);
            centerOn(_imp->minimum, _imp->maximum);
            _imp->currentZoom = 1.;
            return;
        }
        update();
    }
    QWidget::keyReleaseEvent(e);
}


void
ScaleSliderQWidget::seekScalePosition(double v)
{
    if (v < _imp->minimum) {
        v = _imp->minimum;
    }
    if (v > _imp->maximum) {
        v = _imp->maximum;
    }


    if ( (v == _imp->value) && _imp->initialized ) {
        return;
    }
    _imp->value = v;
    if (_imp->initialized) {
        update();
    }
}

void
ScaleSliderQWidget::seekInternal(double v)
{
    if (v < _imp->minimum) {
        v = _imp->minimum;
    }
    if (v > _imp->maximum) {
        v = _imp->maximum;
    }
    if (v == _imp->value) {
        return;
    }
    _imp->value = v;
    if (_imp->initialized) {
        update();
    }
    emit positionChanged(v);
}



void
ScaleSliderQWidget::setMinimumAndMaximum(double min,
                                         double max)
{
    _imp->minimum = min;
    _imp->maximum = max;
    centerOn(_imp->minimum, _imp->maximum);
}

void
ScaleSliderQWidget::centerOn(double left,
                             double right)
{

    double w = right - left;
    _imp->zoomCtx.fill(left - w * 0.05, right + w * 0.05, _imp->zoomCtx.bottom(), _imp->zoomCtx.top());

    update();
}

void
ScaleSliderQWidget::resizeEvent(QResizeEvent* e)
{
    _imp->zoomCtx.setScreenSize(e->size().width(), e->size().height());
    QWidget::resizeEvent(e);
}

void
ScaleSliderQWidget::paintEvent(QPaintEvent* /*e*/)
{
    if (_imp->mustInitializeSliderPosition) {
        centerOn(_imp->minimum, _imp->maximum);
        _imp->mustInitializeSliderPosition = false;
        seekScalePosition(_imp->value);
        _imp->initialized = true;
    }

    ///fill the background with the appropriate style color
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QFontMetrics fontM(*_imp->font);
    p.setPen(_imp->scaleColor);

    QPointF btmLeft = _imp->zoomCtx.toZoomCoordinates(0,height() - 1);
    QPointF topRight = _imp->zoomCtx.toZoomCoordinates(width() - 1, 0);

    if ( btmLeft.x() == topRight.x() ) {
        return;
    }

    /*drawing X axis*/
    double lineYpos = height() - 1 - fontM.height()  - TICK_HEIGHT / 2;
    p.drawLine(0, lineYpos, width() - 1, lineYpos);

    double tickBottom = _imp->zoomCtx.toZoomCoordinates( 0,height() - 1 - fontM.height() ).y();
    double tickTop = _imp->zoomCtx.toZoomCoordinates(0,height() - 1 - fontM.height()  - TICK_HEIGHT).y();
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    const double rangePixel =  width();
    const double range_min = btmLeft.x();
    const double range_max =  topRight.x();
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
    const double minTickSizeTextPixel = fontM.width( QString("00") ); // AXIS-SPECIFIC
    const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
    for (int i = m1; i <= m2; ++i) {
        double value = i * smallTickSize + offset;
        const double tickSize = ticks[i - m1] * smallTickSize;
        const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);
        QColor color(_imp->textColor);
        color.setAlphaF(alpha);
        QPen pen(color);
        pen.setWidthF(1.9);
        p.setPen(pen);

        QPointF tickBottomPos = _imp->zoomCtx.toWidgetCoordinates(value, tickBottom);
        QPointF tickTopPos = _imp->zoomCtx.toWidgetCoordinates(value, tickTop);

        p.drawLine(tickBottomPos,tickTopPos);


        if (tickSize > minTickSizeText) {
            const int tickSizePixel = rangePixel * tickSize / range;
            const QString s = QString::number(value);
            const int sSizePixel =  fontM.width(s);
            if (tickSizePixel > sSizePixel) {
                const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                double alphaText = 1.0; //alpha;
                if (tickSizePixel < sSizeFullPixel) {
                    // when the text size is between sSizePixel and sSizeFullPixel,
                    // draw it with a lower alpha
                    alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                }
                QColor c = _imp->readOnly || !isEnabled() ? Qt::black : _imp->textColor;
                c.setAlphaF(alphaText);
                p.setFont(*_imp->font);
                p.setPen(c);

                QPointF textPos = _imp->zoomCtx.toWidgetCoordinates( value, btmLeft.y() );

                p.drawText(textPos, s);
            }
        }
    }
    double positionValue = _imp->zoomCtx.toWidgetCoordinates(_imp->value,0).x();
    QPointF sliderBottomLeft(positionValue - SLIDER_WIDTH / 2,height() - 1 - fontM.height() / 2);
    QPointF sliderTopRight(positionValue + SLIDER_WIDTH / 2,height() - 1 - fontM.height() / 2 - SLIDER_HEIGHT);

    /*draw the slider*/
    p.setPen(_imp->sliderColor);
    p.fillRect(sliderBottomLeft.x(), sliderBottomLeft.y(), sliderTopRight.x() - sliderBottomLeft.x(), sliderTopRight.y() - sliderBottomLeft.y(),_imp->sliderColor);

    /*draw a black rect around the slider for contrast*/
    p.setPen(Qt::black);

    p.drawLine( sliderBottomLeft.x(),sliderBottomLeft.y(),sliderBottomLeft.x(),sliderTopRight.y() );
    p.drawLine( sliderBottomLeft.x(),sliderTopRight.y(),sliderTopRight.x(),sliderTopRight.y() );
    p.drawLine( sliderTopRight.x(),sliderTopRight.y(),sliderTopRight.x(),sliderBottomLeft.y() );
    p.drawLine( sliderTopRight.x(),sliderBottomLeft.y(),sliderBottomLeft.x(),sliderBottomLeft.y() );
} // paintEvent

void
ScaleSliderQWidget::setReadOnly(bool ro)
{
    _imp->readOnly = ro;
    update();
}


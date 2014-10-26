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
#define SLIDER_HEIGHT 20

ScaleSliderQWidget::ScaleSliderQWidget(double bottom,
                                       double top,
                                       double initialPos,
                                       Natron::ScaleTypeEnum type,
                                       QWidget* parent)
    : QWidget(parent)
      , _zoomCtx()
      , _oldClick()
      , _minimum(bottom)
      , _maximum(top)
      , _type(type)
      , _value(initialPos)
      , _dragging(false)
      , _font( new QFont(NATRON_FONT_ALT, NATRON_FONT_SIZE_8) )
      , _textColor(200,200,200,255)
      , _scaleColor(100,100,100,255)
      , _sliderColor(97,83,30,255)
      , _initialized(false)
      , _mustInitializeSliderPosition(true)
      , _readOnly(false)
      , _ctrlDown(false)
      , _shiftDown(false)
      , _currentZoom(1.)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QSize sizeh = sizeHint();
    _zoomCtx.setScreenSize(sizeh.width(), sizeh.height());
    setFocusPolicy(Qt::ClickFocus);
}

QSize
ScaleSliderQWidget::sizeHint() const
{
    return QSize(150,30);
}

ScaleSliderQWidget::~ScaleSliderQWidget()
{
    delete _font;
}

void
ScaleSliderQWidget::mousePressEvent(QMouseEvent* e)
{
    if (!_readOnly) {
        QPoint newClick =  e->pos();

        _oldClick = newClick;
        QPointF newClick_opengl = _zoomCtx.toZoomCoordinates( newClick.x(),newClick.y() );

        seekInternal( newClick_opengl.x() );
    }
    QWidget::mousePressEvent(e);
}

void
ScaleSliderQWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!_readOnly) {
        QPoint newClick =  e->pos();
        QPointF newClick_opengl = _zoomCtx.toZoomCoordinates( newClick.x(),newClick.y() );

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
        _ctrlDown = true;
        double scale = _shiftDown ? 100. : 10.;
        _currentZoom = scale;
        _zoomCtx.zoomx(_value, 0, scale);
        update();
    } else if (e->key() == Qt::Key_Shift) {
        _shiftDown = true;
        if (_ctrlDown) {
            _zoomCtx.zoomx(_value, 0, 10.);
            _currentZoom = 100.;
        }
        update();
    }
    QWidget::keyPressEvent(e);
}

void
ScaleSliderQWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        _ctrlDown = false;
        _zoomCtx.zoomx(_value, 0, 1. / _currentZoom);
        _currentZoom = 1.;
        update();
    } else if (e->key() == Qt::Key_Shift) {
        _shiftDown = false;
        if (_ctrlDown) {
            _zoomCtx.zoomx(_value, 0, 1. / 10.);
            _currentZoom = 10.;
        } else {
            _zoomCtx.zoomx(_value, 0, 1. / _currentZoom);
            _currentZoom = 1.;
        }
        update();
    }
    QWidget::keyReleaseEvent(e);
}


void
ScaleSliderQWidget::seekScalePosition(double v)
{
    if (v < _minimum) {
        v = _minimum;
    }
    if (v > _maximum) {
        v = _maximum;
    }


    if ( (v == _value) && _initialized ) {
        return;
    }
    _value = v;
    if (_initialized) {
        update();
    }
}

void
ScaleSliderQWidget::seekInternal(double v)
{
    if (v < _minimum) {
        v = _minimum;
    }
    if (v > _maximum) {
        v = _maximum;
    }
    if (v == _value) {
        return;
    }
    _value = v;
    if (_initialized) {
        update();
    }
    emit positionChanged(v);
}



void
ScaleSliderQWidget::setMinimumAndMaximum(double min,
                                         double max)
{
    _minimum = min;
    _maximum = max;
    centerOn(_minimum, _maximum);
}

void
ScaleSliderQWidget::centerOn(double left,
                             double right)
{

    double w = right - left;
    _zoomCtx.fill(left - w * 0.05, right + w * 0.05, _zoomCtx.bottom(), _zoomCtx.top());

    update();
}

void
ScaleSliderQWidget::resizeEvent(QResizeEvent* e)
{
    _zoomCtx.setScreenSize(e->size().width(), e->size().height());
    QWidget::resizeEvent(e);
}

void
ScaleSliderQWidget::paintEvent(QPaintEvent* /*e*/)
{
    if (_mustInitializeSliderPosition) {
        centerOn(_minimum, _maximum);
        _mustInitializeSliderPosition = false;
        seekScalePosition(_value);
        _initialized = true;
    }

    ///fill the background with the appropriate style color
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QFontMetrics fontM(*_font);
    p.setPen(_scaleColor);

    QPointF btmLeft = _zoomCtx.toZoomCoordinates(0,height() - 1);
    QPointF topRight = _zoomCtx.toZoomCoordinates(width() - 1, 0);

    if ( btmLeft.x() == topRight.x() ) {
        return;
    }

    /*drawing X axis*/
    double lineYpos = height() - 1 - fontM.height()  - TICK_HEIGHT / 2;
    p.drawLine(0, lineYpos, width() - 1, lineYpos);

    double tickBottom = _zoomCtx.toZoomCoordinates( 0,height() - 1 - fontM.height() ).y();
    double tickTop = _zoomCtx.toZoomCoordinates(0,height() - 1 - fontM.height()  - TICK_HEIGHT).y();
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
        QColor color(_textColor);
        color.setAlphaF(alpha);
        QPen pen(color);
        pen.setWidthF(1.9);
        p.setPen(pen);

        QPointF tickBottomPos = _zoomCtx.toWidgetCoordinates(value, tickBottom);
        QPointF tickTopPos = _zoomCtx.toWidgetCoordinates(value, tickTop);

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
                QColor c = _readOnly || !isEnabled() ? Qt::black : _textColor;
                c.setAlphaF(alphaText);
                p.setFont(*_font);
                p.setPen(c);

                QPointF textPos = _zoomCtx.toWidgetCoordinates( value, btmLeft.y() );

                p.drawText(textPos, s);
            }
        }
    }
    double positionValue = _zoomCtx.toWidgetCoordinates(_value,0).x();
    QPointF sliderBottomLeft(positionValue - SLIDER_WIDTH / 2,height() - 1 - fontM.height() / 2);
    QPointF sliderTopRight(positionValue + SLIDER_WIDTH / 2,height() - 1 - fontM.height() / 2 - SLIDER_HEIGHT);

    /*draw the slider*/
    p.setPen(_sliderColor);
    p.fillRect(sliderBottomLeft.x(), sliderBottomLeft.y(), sliderTopRight.x() - sliderBottomLeft.x(), sliderTopRight.y() - sliderBottomLeft.y(),_sliderColor);

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
    _readOnly = ro;
    update();
}


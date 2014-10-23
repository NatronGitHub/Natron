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

#ifndef SCALESLIDERQWIDGET_H
#define SCALESLIDERQWIDGET_H

#include <vector>
#include <cmath> // for std::pow()

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

using Natron::ScaleTypeEnum;

class QFont;
class ScaleSliderQWidget
    : public QWidget
{
    Q_OBJECT

public:

    ScaleSliderQWidget(double bottom, // the minimum value
                       double top, // the maximum value
                       double initialPos, // the initial value
                       Natron::ScaleTypeEnum type = Natron::eScaleTypeLinear, // the type of scale
                       QWidget* parent = 0);

    virtual ~ScaleSliderQWidget() OVERRIDE;

    void setMinimumAndMaximum(double min,double max);

    void changeScale(Natron::ScaleTypeEnum type)
    {
        _type = type;
    }

    Natron::ScaleTypeEnum type() const
    {
        return _type;
    }

    double minimum() const
    {
        return _minimum;
    }

    double maximum() const
    {
        return _maximum;
    }

    void setReadOnly(bool ro);

    bool isReadOnly() const
    {
        return _readOnly;
    }

    double getPosition() const
    {
        return _value;
    }

signals:
    void editingFinished();
    void positionChanged(double);

public slots:

    void seekScalePosition(double v);

private:

    void seekInternal(double v);

    /**
     *@brief See toZoomCoordinates in ViewerGL.h
     **/
    QPointF toScaleCoordinates(double x, double y);

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y);

    void centerOn(double left,double right);

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;


    // see ViewerGL.cpp for a full documentation of ZoomContext
    struct ZoomContext
    {
        ZoomContext()
            : bottom(0.)
              , left(0.)
              , zoomFactor(1.)
        {
        }

        QPoint oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
        double bottom; /// the bottom edge of orthographic projection
        double left; /// the left edge of the orthographic projection
        double zoomFactor; /// the zoom factor applied to the current image
        double lastOrthoLeft, lastOrthoBottom, lastOrthoRight, lastOrthoTop; //< remembers the last values passed to the glOrtho call
    };

    ZoomContext _zoomCtx;
    double _minimum,_maximum;
    Natron::ScaleTypeEnum _type;
    double _value;
    bool _dragging;
    QFont* _font;
    QColor _textColor;
    QColor _scaleColor;
    QColor _sliderColor;
    bool _initialized;
    bool _mustInitializeSliderPosition;
    bool _readOnly;
};

#endif // SCALESLIDERQWIDGET_H

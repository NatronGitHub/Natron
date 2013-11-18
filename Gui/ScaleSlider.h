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

#ifndef NATRON_GUI_SCALESLIDER_H_
#define NATRON_GUI_SCALESLIDER_H_

#include <vector>
#include <cmath> // for std::pow()
#include <QtCore/QObject>
#include <QWidget>

#include "Global/Enums.h"

#include "Gui/ViewerGL.h"

using Natron::Scale_Type;

class QFont;
class ScaleSlider : public QGLWidget
{
    Q_OBJECT


    class ZoomContext{

    public:

        ZoomContext():
        _bottom(0.)
        ,_left(0.)
        ,_zoomFactor(1.)
        {}

        QPoint _oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
        double _bottom; /// the bottom edge of orthographic projection
        double _left; /// the left edge of the orthographic projection
        double _zoomFactor; /// the zoom factor applied to the current image

        double _lastOrthoLeft,_lastOrthoBottom,_lastOrthoRight,_lastOrthoTop; //< remembers the last values passed to the glOrtho call

        /*!< the level of zoom used to display the frame*/
        void setZoomFactor(double f){assert(f>0.); _zoomFactor = f;}

        double getZoomFactor() const {return _zoomFactor;}
    };

    ZoomContext _zoomCtx;
    Natron::TextRenderer _textRenderer;
    double _minimum,_maximum;
    Natron::Scale_Type _type;
    double _position;
    std::vector<double> _XValues; // lut where each values is the coordinates of the value mapped on the slider
    bool _dragging;
    QFont* _font;
    QColor _clearColor;
    QColor _majorAxisColor;
    QColor _scaleColor;
    QColor _sliderColor;
    bool _initialized;
    bool _mustInitializeSliderPosition;
public:
    
    ScaleSlider(double bottom, // the minimum value
                double top, // the maximum value
                double initialPos, // the initial value
                Natron::Scale_Type type = Natron::LINEAR_SCALE, // the type of scale
                QWidget* parent=0);
    
    virtual ~ScaleSlider();

    virtual void initializeGL();

    virtual void resizeGL(int width,int height);

    virtual void paintGL();

    virtual void mousePressEvent(QMouseEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual QSize sizeHint() const;

    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font);
    
    void setMinimum(double m){_minimum = m;}
    
    void setMaximum(double m){_maximum = m;}
        
    void changeScale(Natron::Scale_Type type){_type = type;}
    
    Natron::Scale_Type type() const {return _type;}
    
    double minimum() const {return _minimum;}
    
    double maximum() const {return _maximum;}
          
    static void LinearScale1(double xmin ,double xmax, int n,
                             double* xminp, double* xmaxp, double *dist);
    
    static void LinearScale2(double xmin ,double xmax, int n,
                             const std::vector<double> &vint,
                             double* xminp ,double* xmaxp ,double *dist);
    
    static void LogScale1(double xmin ,double xmax ,int n,
                          const std::vector<double> &vint,
                          double* xminp, double* xmaxp ,double *dist);
    
signals:
    
    void positionChanged(double);

public slots:

    void seekScalePosition(double v);
private :

    void seekInternal(double v);

    /**
     *@brief See toImgCoordinates_fast in ViewerGL.h
     **/
    QPointF toImgCoordinates_fast(int x,int y);

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPoint toWidgetCoordinates(double x, double y);

    void drawScale();
};


#endif /* defined(NATRON_GUI_SCALESLIDER_H_) */

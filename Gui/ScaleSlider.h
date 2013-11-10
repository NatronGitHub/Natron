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

#ifndef POWITER_GUI_SCALESLIDER_H_
#define POWITER_GUI_SCALESLIDER_H_

#include <vector>
#include <cmath> // for std::pow()
#include <QtCore/QObject>
#include <QWidget>

#include "Global/Macros.h"
#include "Global/Enums.h"
#include "Global/GlobalDefines.h"



using Natron::Scale_Type;

class ScaleSlider : public QWidget
{
    Q_OBJECT
    
    double _minimum,_maximum;
    int _nbValues;
    Natron::Scale_Type _type;
    double _position;
    std::vector<double> _values;
    std::vector<double> _displayedValues;
    std::vector<double> _XValues; // lut where each values is the coordinates of the value mapped on the slider
    bool _dragging;
    
public:
    
    ScaleSlider(double bottom, // the minimum value
                double top, // the maximum value
                int nbValues, // how many values you want in this interval
                double initialPos, // the initial value
                Natron::Scale_Type type = Natron::LINEAR_SCALE, // the type of scale
                QWidget* parent=0);
    
    virtual ~ScaleSlider();
    
    void setMinimum(double m){_minimum=m;}
    
    void setMaximum(double m){_maximum=m;}
    
    void setPrecision(int i){_nbValues=i;}
    
    void changeScale(Natron::Scale_Type type){_type = type;}
    
    Natron::Scale_Type type() const {return _type;}
    
    double minimum() const {return _minimum;}
    
    double maximum() const {return _maximum;}
    
    int precision() const {return _nbValues;}
    
    
    static void logScale(int N, // number of elements
                         double minimum, //minimum value
                         double maximum, //maximum value
                         double power,//the power used to compute the logscale
                         std::vector<double>* values);// values returned
    
    
    static void linearScale(int N, // number of elements
                            double minimum, //minimum value
                            double maximum, //maximum value
                            std::vector<double>* values); //values
    
    
signals:
    
    void positionChanged(double);

public slots:
    
    void seekScalePosition(double);
    
    
protected:
    
    
    virtual void mousePressEvent(QMouseEvent* e);
    
    virtual void mouseMoveEvent(QMouseEvent* e);
    
    virtual void mouseReleaseEvent(QMouseEvent* e);
    
    virtual void paintEvent(QPaintEvent* e);
    
private:
    
    
    void updateScale();
    
    double toWidgetCoords(double); // input: scale value, output: corresponding coordinate on widget
    
    double toScalePosition(double); // input: coord of widget, output: corresponding scale value
    
    
    
};


#endif /* defined(POWITER_GUI_SCALESLIDER_H_) */

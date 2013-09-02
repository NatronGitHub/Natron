//  Powiter
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

#include "Global/GlobalDefines.h"

#define BORDER_HEIGHT 10
#define BORDER_OFFSET 4
#define TICK_HEIGHT 7

using Powiter::Scale_Type;

class ScaleSlider : public QWidget
{
    Q_OBJECT
    
    double _minimum,_maximum;
    double _nbValues;
    Powiter::Scale_Type _type;
    
public:
    
    ScaleSlider(double bottom,double top,double nbValues,double initialPos,Powiter::Scale_Type type=Powiter::LINEAR_SCALE,
                int nbDisplayedValues=5,QWidget* parent=0);
    virtual ~ScaleSlider();
    /*These functions require a call to XXscale(nb,inf,sup) ,hence the updateScale */
    void setMinimum(double m){_minimum=m; updateScale();}
    void setMaximum(double m){_maximum=m; updateScale();}
    void setPrecision(double i){_nbValues=i; updateScale();}
    void changeScale(Powiter::Scale_Type type){_type = type; updateScale();}
    /*---------------------------------------------*/
    
    Powiter::Scale_Type type() const {return _type;}
    double minimum() const {return _minimum;}
    double maximum() const {return _maximum;}
    double precision() const {return _nbValues;}

    
signals:
    void positionChanged(double);
public slots:
    void seekScalePosition(double);
        
    
protected:
    double _position;
    std::vector<double> _values;
    std::vector<double> _displayedValues;
    std::vector<double> _XValues; // lut where each values is the coordinates of the value mapped on the slider
    bool _dragging;
    void updateScale();
    void fillCoordLut();
    double getScalePosition(double); // input: scale value, output: corresponding coordinate on scale
    double getCoordPosition(double); // opposite

    
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void paintEvent(QPaintEvent* e);
private:
   
    
    static std::vector<double> logScale(int N,float inf,float sup){
        std::vector<double> out;
        for(int i =0;i < N;++i) {
            out.push_back(std::pow((double)i,3)*(sup-inf)/(std::pow((double)(N-1),3)) + inf);
        }
        return out;
    }
    static std::vector<double> expScale(int N,float inf,float sup){
        std::vector<double> out;
        for(int i =0;i < N;++i) {
            out.push_back(std::pow((double)(i),1./2.2)*(sup-inf)/(std::pow((double)(N-1),1./2.2)) + inf);
        }
        return out;
    }
    static std::vector<double> linearScale(int N,float inf,float sup){
        std::vector<double> out;
        double v=inf;
        for(int i =0;i < N;++i) {
            out.push_back(v);
            v +=(sup-inf)/(double)(N-1);
            
        }
        return out;
    }

    
};


#endif /* defined(POWITER_GUI_SCALESLIDER_H_) */

//
//  timeline.h
//  PowiterOsX
//
//  Created by Alexandre on 3/5/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__timeline__
#define __PowiterOsX__timeline__

#include <iostream>
#include <QtWidgets/QWidget>
#include <QtGui/qevent.h>
#include "Superviser/powiterFn.h"
#define BORDER_HEIGHT_ 10
#define BORDER_OFFSET_ 10
#define LINE_START 15
#define TICK_HEIGHT_ 7
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

class TimeSlider : public QWidget{
    Q_OBJECT
    
    int _first,_last,_minimum,_maximum,_current;
    std::vector<int> _values;
    QList<int> _displayedValues;
    std::vector<double> _XValues; // lut where each values is the coordinates of the value mapped on the slider
    int _increment; // 5 or 10 (for displayed values)
    bool _alphaCursor;
    QPointF _Mouse;
    Powiter_Enums::TIMELINE_STATE _state;
    
    std::vector<int> _cached;
    
signals:
    void positionChanged(int);
public slots:
    void seek(int);
    void seek(double d){seek((int)d);}
    void changeFirstAndLast(QString); // for the spinbox
        
public:
    
    TimeSlider(QWidget* parent=0);
    virtual ~TimeSlider(){}
    
    void addCachedFrame(int f);
    void removeCachedFrame();
    void clearCachedFrames(){_cached.clear();repaint();}

    
    void setFrameRange(int min,int max,bool initBoundaries=true){
        _minimum=min;
        _maximum=max;
        if(initBoundaries){
            _first=min;
            _last=max;
        }
        if((_maximum - _minimum)==0){
            _minimum = 0;
            _maximum = 100;
            _first =0;
            _last=0;
            _current = 0;
        }else{
            
            _current=min;
        }
        updateScale();
    }
    int firstFrame() const{return _first;}
    int lastFrame() const{return _last;}
    
protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void paintEvent(QPaintEvent* e);
    virtual void enterEvent(QEvent* e);
    virtual void leaveEvent(QEvent* e);
private:
    double getScalePosition(double); // input: scale value, output: corresponding coordinate on scale
    double getCoordPosition(double); // opposite
    void fillCoordLut();
    void updateScale();
    void drawTicks(QPainter* p,QColor& scaleColor);
    void changeFirst(int);
    void changeLast(int);

};

#endif /* defined(__PowiterOsX__timeline__) */

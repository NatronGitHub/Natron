//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__timeline__
#define __PowiterOsX__timeline__

#include <iostream>
#include <QtWidgets/QWidget>
#include "Superviser/powiterFn.h"
#define BORDER_HEIGHT_ 10
#define BORDER_OFFSET_ 10
#define LINE_START 15
#define TICK_HEIGHT_ 7
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

class QMouseEvent;
class TimeLine : public QWidget{
    Q_OBJECT
    
    int _first,_last; // the first and last frames, bounded by the user
    int _minimum,_maximum; // the extremas displayed on the timeline
    int _current; // the current frame
    
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
    
    TimeLine(QWidget* parent=0);
    virtual ~TimeLine(){}
    
    /*Tells the timeline to indicate that the frame f is cached*/
    void addCachedFrame(int f);
    
    /*Removes the last recently used cache frame. Called by the ViewerCache*/
    void removeCachedFrame();
    
    /*clears out cached frames*/
    void clearCachedFrames(){_cached.clear();repaint();}

    /*initialises the frame range displayed*/
    void setFrameRange(int min,int max);
    
    /*initialises the boundaries on the timeline*/
    void setBoundaries(int first,int last);
    
    int firstFrame() const{return _first;}
    int lastFrame() const{return _last;}
    int currentFrame() const{return _current;}
    
protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void paintEvent(QPaintEvent* e);
    virtual void enterEvent(QEvent* e);
    virtual void leaveEvent(QEvent* e);
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
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

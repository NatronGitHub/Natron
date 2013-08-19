//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include "Gui/Timeline.h"

using namespace std;
using namespace Powiter;
TimeLine::TimeLine(QWidget* parent):QWidget(parent),
_first(0),_last(100),_minimum(0),_maximum(100),_current(0),_alphaCursor(false),_state(IDLE)
{
    
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
   // setMinimumSize(500,40);
    for(int i =0;i <=100;i++) _values.push_back(i);
    _displayedValues << 0 << 10 <<  20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100;
    _increment=10;
    setMouseTracking(true);
        
}

QSize TimeLine::minimumSizeHint() const{
    return QSize(500,40);
}
QSize TimeLine::sizeHint() const{
    return QSize(500,40);
}

void TimeLine::updateScale(){
    _values.clear();
    _displayedValues.clear();
//    int _maximum = _first;
//    int _minimum = _last;
    
    if((_maximum - _minimum)==0){
        for(int i =0;i <=100;i++) _values.push_back(i);
        _displayedValues << 0 << 10 <<  20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100;
        _increment=10;
        
    }
    else if((_maximum- _minimum) <=50){ // display every multiple of 5
        int c = _minimum;
        while(c%5!=0 && c>=0){
            c--;
        }
        int realMin = c;
        c = _maximum;
        while(c%5!=0){
            c++;
        }
        int realMax = c;
        for(int i = realMin ; i <= realMax;i++) _values.push_back(i);
        c=realMin;


        _increment=5;
        while(c%5!=0 && c<=_maximum){
            c++;
        }
        if( c != _maximum){
            while(c <realMax){
                _displayedValues << c;
                c+=5;
            }
        }
        _displayedValues << realMax;

    }else{ // display only multiple of 10
        int c = _minimum;
        while(c%10!=0 && c>=0){
            c--;
        }
        int realMin = c;
        c = _maximum;
        while(c%10!=0){
            c++;
        }
        int realMax = c;
        for(int i = realMin ; i <= realMax;i++) _values.push_back(i);
        c=realMin;


        _increment=10;
        while(c%10!=0 && c<=_maximum){
            c++;
        }
        if( c != _maximum){
            while(c <realMax){
                _displayedValues << c;
                c+=10;
            }
        }
        _displayedValues << realMax;

    }
}

void TimeLine::paintEvent(QPaintEvent *e){
    Q_UNUSED(e);
    
    QColor bg(50,50,50,255);
    int w = size().width();
    int h = size().height();
    QColor scaleColor(100,100,100);
    QPainter p(this);
    p.setWindow(0, 0, w, h);
    p.fillRect(0, 0, w, h,bg );
    
    QFont f("Times",10);
    p.setPen(scaleColor);
    p.setFont(f);
    p.drawLine(BORDER_OFFSET_, LINE_START, BORDER_OFFSET_, LINE_START+BORDER_HEIGHT_); // left border
    p.drawLine(w-BORDER_OFFSET_,LINE_START,w-BORDER_OFFSET_,LINE_START+BORDER_HEIGHT_); // right border
    p.drawLine(BORDER_OFFSET_,BORDER_HEIGHT_+LINE_START,w-BORDER_OFFSET_,BORDER_HEIGHT_+LINE_START); // horizontal line
    fillCoordLut();
    // drawing ticks & sub-ticks
    drawTicks(&p,scaleColor);
    QPolygonF cursorPoly;
    QColor cursorColor(243,149,0);
    p.setPen(cursorColor);
    double cursorX = getCoordPosition(_current);
    double lastX = getCoordPosition(_last);
    double firstX = getCoordPosition(_first);
    
    if(_alphaCursor){
        double currentPos = _Mouse.x();
        QColor _currentColor(243,149,0,100);
        p.setPen(_currentColor);
        QPolygonF currentPoly;
        currentPoly.push_back(QPointF(currentPos ,BORDER_HEIGHT_+LINE_START - 2));
        currentPoly.push_back(QPointF(currentPos - CURSOR_WIDTH/2 ,BORDER_HEIGHT_+LINE_START - 2 - CURSOR_HEIGHT));
        currentPoly.push_back(QPointF(currentPos + CURSOR_WIDTH/2,BORDER_HEIGHT_+LINE_START -2 -CURSOR_HEIGHT));
        QString mouseNumber(QString::number(getScalePosition(_Mouse.x())));
        QPointF mouseNbTextPos(currentPos,BORDER_HEIGHT_+LINE_START-CURSOR_HEIGHT-5);
        if(mouseNumber.size()==1){
            p.drawText(QPointF(mouseNbTextPos.x()-2,mouseNbTextPos.y()), mouseNumber,'g',3);
            
        }else if(mouseNumber.size()==2){
            p.drawText(QPointF(mouseNbTextPos.x()-6,mouseNbTextPos.y()), mouseNumber,'g',3);
            
        }else if(mouseNumber.size()==3){
            p.drawText(QPointF(mouseNbTextPos.x()-7,mouseNbTextPos.y()), mouseNumber,'g',3);
            
        }
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath currentPath_;
        currentPath_.addPolygon(currentPoly);
        currentPath_.closeSubpath();
        p.drawPath(currentPath_);
        p.fillPath(currentPath_, _currentColor);
        p.setRenderHint(QPainter::Antialiasing,false);
        p.setPen(cursorColor);


    }
    
    
    cursorPoly.push_back(QPointF(cursorX ,BORDER_HEIGHT_+LINE_START - 2));
    cursorPoly.push_back(QPointF(cursorX - CURSOR_WIDTH/2 ,BORDER_HEIGHT_+LINE_START - 2 - CURSOR_HEIGHT));
    cursorPoly.push_back(QPointF(cursorX + CURSOR_WIDTH/2,BORDER_HEIGHT_+LINE_START -2 -CURSOR_HEIGHT));
    QString curNumber(QString::number(_current));
    QPointF textPos(cursorX,BORDER_HEIGHT_+LINE_START-CURSOR_HEIGHT-5);
    if(curNumber.size()==1){
        p.drawText(QPointF(textPos.x()-2,textPos.y()), curNumber,'g',3);
        
    }else if(curNumber.size()==2){
        p.drawText(QPointF(textPos.x()-6,textPos.y()), curNumber,'g',3);
        
    }else if(curNumber.size()==3){
        p.drawText(QPointF(textPos.x()-7,textPos.y()), curNumber,'g',3);
        
    }
    QColor boundariesColor(207,69,6);
    p.setPen(boundariesColor);
    QString firstNumber(QString::number(_first));
    QPointF firstPos(firstX,BORDER_HEIGHT_+LINE_START-CURSOR_HEIGHT-5);
    if(firstNumber.size()==1){
        p.drawText(QPointF(firstPos.x()-2,firstPos.y()), firstNumber,'g',3);
        
    }else if(firstNumber.size()==2){
        p.drawText(QPointF(firstPos.x()-6,firstPos.y()), firstNumber,'g',3);
        
    }else if(firstNumber.size()==3){
        p.drawText(QPointF(firstPos.x()-7,firstPos.y()), firstNumber,'g',3);
        
    }
    
    QString lastNumber(QString::number(_last));
    QPointF lastPos(lastX,BORDER_HEIGHT_+LINE_START-CURSOR_HEIGHT-5);
    if(lastNumber.size()==1){
        p.drawText(QPointF(lastPos.x()-2,lastPos.y()), lastNumber,'g',3);
        
    }else if(lastNumber.size()==2){
        p.drawText(QPointF(lastPos.x()-6,lastPos.y()), lastNumber,'g',3);
        
    }else if(lastNumber.size()==3){
        p.drawText(QPointF(lastPos.x()-7,lastPos.y()), lastNumber,'g',3);
        
    }
    
    QPainterPath cursor;
    p.setRenderHint(QPainter::Antialiasing);
    cursor.addPolygon(cursorPoly);
    cursor.closeSubpath();
    p.setPen(cursorColor);
    p.drawPath(cursor);
    p.fillPath(cursor, cursorColor);
    
    
    
    
    QPolygonF lastPoly,firstPoly;
    
    lastPoly.push_back(QPointF(lastX,BORDER_HEIGHT_+LINE_START));
    lastPoly.push_back(QPointF(lastX,BORDER_HEIGHT_+LINE_START  - CURSOR_HEIGHT));
    lastPoly.push_back(QPointF(lastX-CURSOR_HEIGHT,BORDER_HEIGHT_+LINE_START ));
    firstPoly.push_back(QPointF(firstX,BORDER_HEIGHT_+LINE_START));
    firstPoly.push_back(QPointF(firstX,BORDER_HEIGHT_+LINE_START  - CURSOR_HEIGHT));
    firstPoly.push_back(QPointF(firstX+CURSOR_HEIGHT,BORDER_HEIGHT_+LINE_START ));
    QPainterPath firstP,lastP;
    firstP.addPolygon(firstPoly);
    firstP.closeSubpath();
    lastP.addPolygon(lastPoly);
    lastP.closeSubpath();
    p.setPen(boundariesColor);
    p.drawPath(firstP);
    p.drawPath(lastP);
    p.fillPath(firstP,boundariesColor);
    p.fillPath(lastP, boundariesColor);
    
    
    QColor cachedColor(143,201,103,255);
    double offset = (double)((w-BORDER_OFFSET_) - BORDER_OFFSET_ )/(double)(_displayedValues.size()-1);
    if(_increment==5)
        offset/=5.0;
    else if(_increment==10)
        offset/=10.0;
    p.setPen(cachedColor);
    for(U32 i =0 ; i < _cached.size() ; i++){
        double pos = getCoordPosition(_cached[i]);

        p.drawLine(pos-offset/2.0,BORDER_HEIGHT_+LINE_START+2,pos+offset/2.0,BORDER_HEIGHT_+LINE_START+2);
        p.drawLine(pos-offset/2.0,BORDER_HEIGHT_+LINE_START+3,pos+offset/2.0,BORDER_HEIGHT_+LINE_START+3);
        p.drawLine(pos-offset/2.0,BORDER_HEIGHT_+LINE_START+4,pos+offset/2.0,BORDER_HEIGHT_+LINE_START+4);
      

    }

    
}
void TimeLine::drawTicks(QPainter *p,QColor& scaleColor){
    int w = size().width();
    double scaleW = (w-BORDER_OFFSET_) - BORDER_OFFSET_ ;
    double incr=1;
    if(_increment==5){
        incr=((scaleW)/(double)(_displayedValues.size()-1))/5.f;

    }else if(_increment==10){
        incr=((scaleW)/(double)(_displayedValues.size()-1))/10.f;
    }
    QPointF pos(BORDER_OFFSET_-1,23+LINE_START);
    for(int i =0;i<_displayedValues.size();i++){
        p->setPen(QColor(200,200,200));
        QString nbStr = QString::number(_displayedValues[i]);
        if(nbStr.size()==1){
            p->drawText(QPointF(pos.x()-2,pos.y()), nbStr,'g',3);

        }else if(nbStr.size()==2){
            p->drawText(QPointF(pos.x()-4,pos.y()), nbStr,'g',3);

        }else if(nbStr.size()==3){
            p->drawText(QPointF(pos.x()-7,pos.y()), nbStr,'g',3);
            
        }
        p->setPen(scaleColor);
        if(i != _displayedValues.size()-1){
            for(int j=1;j<_increment;j++){
                p->drawLine(pos.x()+j*incr,2+BORDER_HEIGHT_+LINE_START,pos.x()+j*incr,BORDER_HEIGHT_+LINE_START-TICK_HEIGHT_/2);
            }
        }
        
        if(i>0 && i<_displayedValues.size()-1){
            p->drawLine(pos.x(),2+BORDER_HEIGHT_+LINE_START,pos.x(),2+BORDER_HEIGHT_+LINE_START-TICK_HEIGHT_);
        }
        pos.setX(pos.x()+incr*_increment);
    }
    
}

double TimeLine::getScalePosition(double pos){
    if(pos <= _XValues [0])
        return _values[0];
    if(pos >= _XValues[_XValues.size()-1])
        return _values[_XValues.size()-1];
    for(unsigned int i=0;i<_values.size();i++){
        if(i<_XValues.size()-1 &&  pos >= _XValues[i] && pos < _XValues[i+1]){
            return _values[i];
        }
        if(i == _XValues.size()-1 ){
            return  _values[i];
            
        }
    }
    return -1;

}
double TimeLine::getCoordPosition(double pos){
    if(pos < _values[0]){
        return _XValues[0];
    }
    if(pos > _values[_values.size()-1]){
        return _XValues[_XValues.size()-1];
    }
    for(unsigned int i=0;i<_values.size();i++){
        if(i<_values.size()-1 &&  pos >= _values[i] && pos < _values[i+1]){
            return  _XValues[i];
        }
        if(i == _values.size()-1 ){
            return  _XValues[i];
        }
    }
    return -1;
}
void TimeLine::changeFirst(int v){
    _first = v;
    repaint();
}
void TimeLine::changeLast(int v){
    _last = v;
    repaint();
}

void TimeLine::fillCoordLut(){
    _XValues.clear();
    double c = BORDER_OFFSET_;
    double scaleW = (size().width()-BORDER_OFFSET_) - BORDER_OFFSET_ ;
    double incr= scaleW/(double)(_values.size()-1);
    for(unsigned int i=0;i<_values.size();i++){
        _XValues.push_back(c);
        c+=incr;
    }

}
void TimeLine::seek(int v){
    if(v >=_first && v<=_last)
        _current = v;
    repaint();
}

void TimeLine::seek_notSlot(int v){
    if(v >=_first && v<=_last)
        _current = v;
    QMetaObject::invokeMethod(this, "repaint", Qt::QueuedConnection);
}

void TimeLine::changeFirstAndLast(QString str){
    Q_UNUSED(str);
}

void TimeLine::mousePressEvent(QMouseEvent* e){
    
    int c = getScalePosition(e->x());
    if(e->modifiers().testFlag(Qt::ControlModifier)){
        _state = DRAGGING_BOUNDARY;
        int firstPos,lastPos;
        firstPos = getCoordPosition(_first);
        lastPos = getCoordPosition(_last);
        if(abs( e->x()-firstPos ) > abs(e->x() - lastPos) ){ // moving last frame anchor
            if( c >= _minimum && c<=_maximum){
                _last = c;
            }
        }else{ // moving first frame anchor
            if( c >= _minimum && c<=_maximum){
                _first = c;
            }
        }
            
    }else{
        _state = DRAGGING_CURSOR;
        if(c >= _first && c <=_last){
            _current = c;
            emit positionChanged(_current);
        }
    }
    
    repaint();
    QWidget::mousePressEvent(e);

}
void TimeLine::mouseMoveEvent(QMouseEvent* e){
    _alphaCursor=true;
    _Mouse = e->pos();
    if(_state==DRAGGING_CURSOR){
        int c = getScalePosition(e->x());
        if(c >= _first && c <=_last){
            _current = c;
            emit positionChanged(_current);
        }
    }else if(_state == DRAGGING_BOUNDARY){
        int c = getScalePosition(e->x());
        int firstPos,lastPos;
        firstPos = getCoordPosition(_first);
        lastPos = getCoordPosition(_last);
        if(abs( e->x()-firstPos ) > abs(e->x() - lastPos) ){ // moving last frame anchor
            if( c >= _minimum && c<=_maximum && c >= _first){
                _last = c;
            }
        }else{ // moving first frame anchor
            if( c >= _minimum && c<=_maximum && c<=_last){
                _first = c;
            }
        }

    }
    QWidget::mouseMoveEvent(e);
    repaint();

}
void TimeLine::enterEvent(QEvent* e){
    _alphaCursor = true;
   // grabMouse();
    QWidget::enterEvent(e);
};
void TimeLine::leaveEvent(QEvent* e){
    _alphaCursor = false;
    repaint();
   // releaseMouse();
    QWidget::leaveEvent(e);
}


void TimeLine::mouseReleaseEvent(QMouseEvent* e){
    _state = IDLE;
    QWidget::mouseReleaseEvent(e);
}


void TimeLine::addCachedFrame(int f){
    _cached.push_back(f);
 //  repaint();
}
void TimeLine::removeCachedFrame(){
    _cached.erase(_cached.begin());
   // repaint();
}

void TimeLine::setFrameRange(int min,int max){
    _minimum=min;
    _maximum=max;
    updateScale();
}
/*initialises the boundaries on the timeline*/
void TimeLine::setBoundaries(int first,int last){
    _first = first;
    _last = last;
    repaint();
}


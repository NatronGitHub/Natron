//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "ScaleSlider.h"

#include <qlayout.h>
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QPaintEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

using namespace std;

ScaleSlider::ScaleSlider(double bottom,double top,double nbValues,double initialPos,Powiter::Scale_Type type,int nbDisplayedValues,QWidget* parent):
QWidget(parent),
_minimum(bottom),
_maximum(top),
_nbValues(nbValues),
_type(type),
_dragging(false)
{

    this->setContentsMargins(0, 0, 0, 0);    
    for(int i =0;i<nbDisplayedValues;++i) _displayedValues.push_back(0);
    updateScale();
    _position = initialPos;
    setMinimumWidth(150);
}

ScaleSlider::~ScaleSlider(){
    
}

void ScaleSlider::updateScale(){
    if (_type == Powiter::LINEAR_SCALE) {
        _values = linearScale(_nbValues, _minimum, _maximum);
    }else if(_type == Powiter::LOG_SCALE){
        _values = logScale(_nbValues, _minimum, _maximum);
    }else if(_type == Powiter::EXP_SCALE){
        _values = expScale(_nbValues, _minimum, _maximum);
    }
    double incr = (double)(_values.size())/(double)(_displayedValues.size()-1);
    double index = 0;
    for(U32 i =0;i< _displayedValues.size();++i) {
        if(ceil(index) >= _values.size()){ _displayedValues[i]=_values.back();index+=incr; continue;}
        if(floor(index) <=0){_displayedValues[i]=_values[0];index+=incr;continue;}
        if(floor(index)==index){_displayedValues[i]=_values[index];index+=incr;continue;}
        _displayedValues[i]= _values[floor(index)]*(ceil(index)-index)+_values[ceil(index)]*(index-floor(index));
        index+=incr;
    }
}
void ScaleSlider::paintEvent(QPaintEvent *e){
    Q_UNUSED(e);
    QColor bg(50,50,50,255);
    int w = size().width();
    int h = size().height();
    QColor scaleColor(100,100,100);
    QPainter p(this);
    p.setWindow(0, 0, w, h);
    p.fillRect(0, 0, w, h,bg );
    
    QFont f("Times",8);
    p.setPen(scaleColor);
    p.setFont(f);
    p.drawLine(BORDER_OFFSET, 0, BORDER_OFFSET, 0+BORDER_HEIGHT); // left border
    p.drawLine(w-BORDER_OFFSET,0,w-BORDER_OFFSET,0+BORDER_HEIGHT); // right border
    p.drawLine(BORDER_OFFSET,BORDER_HEIGHT,w-BORDER_OFFSET,BORDER_HEIGHT); // horizontal line
    double scaleW = (w-BORDER_OFFSET) - BORDER_OFFSET ;
    double incr =(scaleW-5)/(double)(_displayedValues.size()-1);
    QPointF pos(BORDER_OFFSET-1,20);
    fillCoordLut();
    // drawing ticks & sub-ticks   
    for(unsigned int i =0;i<_displayedValues.size();++i) {
        p.drawText(pos, QString::number(_displayedValues[i],'g',3));
        if(pos.x()-incr > 2) p.drawLine(pos.x()-incr/2,2+BORDER_HEIGHT,pos.x()-incr/2,BORDER_HEIGHT-TICK_HEIGHT/2);

        if(i>0 && i<_displayedValues.size()-1){
            p.drawLine(pos.x(),2+BORDER_HEIGHT,pos.x(),2+BORDER_HEIGHT-TICK_HEIGHT);
        }
        pos.setX(pos.x()+incr);
    }
    //drawing cursor
    double cursorPos = getCoordPosition(_position);
    p.fillRect(cursorPos-2, BORDER_HEIGHT-TICK_HEIGHT, 4, 2*TICK_HEIGHT,QColor(97,83,30));
    p.setPen(Qt::black);
    p.drawRect(cursorPos-3,BORDER_HEIGHT-TICK_HEIGHT-1,5,2*TICK_HEIGHT+2);
}
void ScaleSlider::fillCoordLut(){
    _XValues.clear();
    double c = BORDER_OFFSET;
    double scaleW = (size().width()-BORDER_OFFSET) - BORDER_OFFSET ;
    double incr= scaleW/(double)(_values.size()-1);
    for(unsigned int i=0;i<_values.size();++i) {
        _XValues.push_back(c);
        c+=incr;
    }
   
}

void ScaleSlider::mousePressEvent(QMouseEvent* e){
    _position = getScalePosition(e->x());
    _dragging=true;
    emit positionChanged(_position);
    repaint();
    
    
}
void ScaleSlider::mouseMoveEvent(QMouseEvent* e){
    if(_dragging){
        _position = getScalePosition(e->x());
        emit positionChanged(_position);
        repaint();
    }
}
void ScaleSlider::seekScalePosition(double d){
    bool loop = true;
    if(d < _values [0]){
        _position= _values[0];
        loop=false;
    }
    if(d > _values[_values.size()-1]){
        _position= _values[_values.size()-1];
        loop=false;
    }
    if(loop){
        for(unsigned int i=0;i<_values.size();++i) {
            if(i<_values.size()-1 &&  d >= _values[i] && d < _values[i+1]){
                _position= _values[i];
                break;
            }
            if(i == _values.size()-1 ){
                _position=  _values[i];
                break;
            }
        }
    }
    repaint();
}

void ScaleSlider::mouseReleaseEvent(QMouseEvent* e){
    Q_UNUSED(e);
    _dragging=false;
}

double ScaleSlider::getScalePosition(double pos){
    if(pos <= _XValues [0])
        return _values[0];
    if(pos >= _XValues[_XValues.size()-1])
        return _values[_XValues.size()-1];
    for(unsigned int i=0;i<_values.size();++i) {
        if(i<_XValues.size()-1 &&  pos >= _XValues[i] && pos < _XValues[i+1]){
            return _values[i];
        }
        if(i == _XValues.size()-1 ){
            return  _values[i];
            
        }
    }
    return -1;
}

double ScaleSlider::getCoordPosition(double pos){
    
    if(pos < _values[0]){
        return _XValues[0];
        
    }
    if(pos > _values[_values.size()-1]){
        return _XValues[_XValues.size()-1];
        
    }
    
    for(unsigned int i=0;i<_values.size();++i) {
        if(i<_values.size()-1 &&  pos >= _values[i] && pos < _values[i+1]){
            return  _XValues[i];
            
        }
        if(i == _values.size()-1 ){
            return  _XValues[i];
            
        }
        
    }
    
    return -1;
}

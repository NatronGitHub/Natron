//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Gui/FeedbackSpinBox.h"

FeedBackSpinBox::FeedBackSpinBox(QWidget* parent,bool mode):QLineEdit(parent),_mode(mode),_decimals(1),_increment(1.0),_mini(0),_maxi(99)
{
//    if(_mode){ // double spinbox
//        _validator=new QDoubleValidator(this);
//        static_cast<QDoubleValidator*>(_validator)->setDecimals(_decimals);
//        
//    }else{ // int spinbox
//        _validator=new QIntValidator(this);
//    }
    QObject::connect(this, SIGNAL(returnPressed()), this, SLOT(interpretReturn()));
//    setStyleSheet("selection-color: rgba(255, 255, 255, 0);background-color: rgba(81,81,81,255);"
//                 "color:rgba(200,200,200,255);");
    setValue(0);
    setMaximumWidth(50);
    setMinimumWidth(35);
    
    decimals(_decimals);
}

void FeedBackSpinBox::setValue(double d){
    clear();
    QString str;
    if(_mode)
        str.setNum(d);
    else
        str.setNum((int)d);
    insert(str);
}
void FeedBackSpinBox::interpretReturn(){
    bool ok;
    emit valueChanged(text().toDouble(&ok));
}

void FeedBackSpinBox::wheelEvent(QWheelEvent *e){
    bool ok;
    double cur= text().toDouble(&ok);
    clear();
    if(e->delta()>0){
        if(cur+_increment <= _maxi)
            cur+=_increment;
    }else{
        if(cur-_increment >= _mini)
            cur-=_increment;
    }
    QString str;
    if(_mode)
        str.setNum(cur);
    else
        str.setNum((int)cur);
    
    insert(str);
    emit valueChanged(cur);
}

void FeedBackSpinBox::keyPressEvent(QKeyEvent *e){
    bool ok;
    double cur= text().toDouble(&ok);
    if(e->key() == Qt::Key_Up){
        clear();
        if(cur+_increment <= _maxi)
            cur+=_increment;
        QString str;
        if(_mode)
            str.setNum(cur);
        else
            str.setNum((int)cur);
        insert(str);
        emit valueChanged(cur);
    }else if(e->key() == Qt::Key_Down){
        clear();
        if(cur-_increment >= _mini)
            cur-=_increment;
        QString str;
        if(_mode)
            str.setNum(cur);
        else
            str.setNum((int)cur);
        insert(str);
        emit valueChanged(cur);
    }
    QLineEdit::keyPressEvent(e);
}
//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "SpinBox.h"

#include "Global/Macros.h"
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QWheelEvent>
#include <QtGui/QDoubleValidator>
#include <QtGui/QIntValidator>

SpinBox::SpinBox(QWidget* parent,SPINBOX_TYPE type):
LineEdit(parent)
,_type(type)
,_decimals(2)
,_increment(1.0)
,_mini(0)
,_maxi(99)
,_doubleValidator(0)
,_intValidator(0)
{
    
    if(type == DOUBLE_SPINBOX){
        _doubleValidator = new QDoubleValidator;
        _doubleValidator->setTop(99);
        _doubleValidator->setBottom(0);
    }else{
        _intValidator = new QIntValidator;
        _intValidator->setTop(99);
        _intValidator->setBottom(0);
    }
    QObject::connect(this, SIGNAL(returnPressed()), this, SLOT(interpretReturn()));
    setValue(0);
    setMaximumWidth(50);
    setMinimumWidth(35);
    decimals(_decimals);
}
SpinBox::~SpinBox(){
    if(_type == DOUBLE_SPINBOX){
        delete _doubleValidator;
    }else{
        delete _intValidator;
    }
}
void SpinBox::setValue(double d){
    clear();
    QString str;
    if(_type == DOUBLE_SPINBOX)
        str.setNum(d,'f',_decimals);
    else
        str.setNum((int)d);
    insert(str);
    home(false);
}
void SpinBox::interpretReturn(){
    emit valueChanged(text().toDouble());
}

void SpinBox::mousePressEvent(QMouseEvent* e){
    LineEdit::mousePressEvent(e);
    setFocus();
}
void SpinBox::wheelEvent(QWheelEvent *e){
    setFocus();
    if(isEnabled() && !isReadOnly()){
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
        if(_type == DOUBLE_SPINBOX)
            str.setNum(cur,'f',_decimals);
        else
            str.setNum((int)cur);
        
        insert(str);
        emit valueChanged(cur);
    }
}

void SpinBox::keyPressEvent(QKeyEvent *e){
    if(isEnabled() && !isReadOnly()){
        bool ok;
        double cur= text().toDouble(&ok);
        if(e->key() == Qt::Key_Up){
            clear();
            if(cur+_increment <= _maxi)
                cur+=_increment;
            QString str;
            if(_type == DOUBLE_SPINBOX)
                str.setNum(cur,'f',_decimals);
            else
                str.setNum((int)cur);
            insert(str);
            emit valueChanged(cur);
        }else if(e->key() == Qt::Key_Down){
            clear();
            if(cur-_increment >= _mini)
                cur-=_increment;
            QString str;
            if(_type == DOUBLE_SPINBOX)
                str.setNum(cur,'f',_decimals);
            else
                str.setNum((int)cur);
            insert(str);
            emit valueChanged(cur);
        }else{
            double oldValue = value();
            QLineEdit::keyPressEvent(e);
            if (_type == DOUBLE_SPINBOX) {
                QString txt = text();
                int tmp;
                QValidator::State st = _doubleValidator->validate(txt,tmp);
                if(st == QValidator::Invalid){
                    setValue(oldValue);
                }
            }else{
                QString txt = text();
                int tmp;
                QValidator::State st = _intValidator->validate(txt,tmp);
                if(st == QValidator::Invalid){
                    setValue(oldValue);
                }
            }
            
        }
    }
}

void SpinBox::decimals(int d){
    _decimals=d;
    setMaxLength(_decimals+3);
    if(_type == DOUBLE_SPINBOX){
        _doubleValidator->setDecimals(d);
    }
}
void SpinBox::setMaximum(double t){
    _maxi = t;
    if(_type == DOUBLE_SPINBOX){
        _doubleValidator->setTop(t);
    }else{
        _intValidator->setTop(t);
    }
    
}
void SpinBox::setMinimum(double b){
    _mini = b;
    if(_type == DOUBLE_SPINBOX){
        _doubleValidator->setBottom(b);
    }else{
        _intValidator->setBottom(b);
    }
}
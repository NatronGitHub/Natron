//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "SpinBox.h"

#include <cfloat>
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
,_mini()
,_maxi()
,_doubleValidator(0)
,_intValidator(0)
{

    if(type == DOUBLE_SPINBOX){
        _mini.setValue<double>(-DBL_MAX);
        _maxi.setValue<double>(DBL_MAX);
        _doubleValidator = new QDoubleValidator;
        _doubleValidator->setTop(DBL_MAX);
        _doubleValidator->setBottom(-DBL_MAX);
    }else{
        _intValidator = new QIntValidator;
        _mini.setValue<int>(INT_MIN);
        _maxi.setValue<int>(INT_MAX);
        _intValidator->setTop(INT_MAX);
        _intValidator->setBottom(INT_MIN);
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
}

void SpinBox::wheelEvent(QWheelEvent *e) {
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    if(isEnabled() && !isReadOnly()){
        bool ok;
        double cur = text().toDouble(&ok);
        clear();
        double maxiD,miniD;
        if(_type == DOUBLE_SPINBOX){
            maxiD = _maxi.toDouble();
            miniD = _mini.toDouble();
        }else{
            maxiD = _maxi.toInt();
            miniD = _mini.toInt();

        }
        if(e->delta()>0){
            if(cur+_increment <= maxiD)
                cur+=_increment;
        }else{
            if(cur-_increment >= miniD)
                cur-=_increment;
        }
        if(cur < miniD || cur > maxiD)
            return;
        QString str;
        if(_type == DOUBLE_SPINBOX){
            str.setNum(cur,'f',_decimals);

        }else{
            str.setNum((int)cur);
        }
        
        insert(str);
        emit valueChanged(cur);
    }
}

void SpinBox::focusOutEvent(QFocusEvent * /*event*/){
   emit valueChanged(text().toDouble());
}

void SpinBox::keyPressEvent(QKeyEvent *e){
    if(isEnabled() && !isReadOnly()){
        bool ok;
        double cur= text().toDouble(&ok);
        double maxiD,miniD;
        if(_type == DOUBLE_SPINBOX){
            maxiD = _maxi.toDouble();
            miniD = _mini.toDouble();
        }else{
            maxiD = _maxi.toInt();
            miniD = _mini.toInt();

        }
        if(e->key() == Qt::Key_Up){
            clear();

            if(cur+_increment <= maxiD)
                cur+=_increment;
            if(cur < miniD || cur > maxiD)
                return;
            QString str;
            if(_type == DOUBLE_SPINBOX)
                str.setNum(cur,'f',_decimals);
            else
                str.setNum((int)cur);
            insert(str);
            emit valueChanged(cur);
        }else if(e->key() == Qt::Key_Down){
            clear();
            if(cur-_increment >= miniD)
                cur-=_increment;
            if(cur < miniD || cur > maxiD)
                return;
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
                if(st == QValidator::Invalid || txt.toDouble() < miniD || txt.toDouble() > maxiD){
                    setValue(oldValue);
                }
            }else{
                QString txt = text();
                int tmp;
                QValidator::State st = _intValidator->validate(txt,tmp);
                if(st == QValidator::Invalid || txt.toDouble() < miniD || txt.toDouble() > maxiD){
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
    if(_type == DOUBLE_SPINBOX){
        _maxi.setValue<double>(t);
        _doubleValidator->setTop(t);
    }else{
        _maxi.setValue<int>((int)t);
        _intValidator->setTop(t);
    }
    
}
void SpinBox::setMinimum(double b){
    if(_type == DOUBLE_SPINBOX){
        _mini.setValue<double>(b);
        _doubleValidator->setBottom(b);
    }else{
        _mini.setValue<int>(b);
        _intValidator->setBottom(b);
    }
}

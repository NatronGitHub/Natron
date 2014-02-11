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

CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QWheelEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QDoubleValidator>
#include <QtGui/QIntValidator>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5

#include "Global/Macros.h"

SpinBox::SpinBox(QWidget* parent,SPINBOX_TYPE type):
LineEdit(parent)
,_type(type)
,_decimals(2)
,_increment(1.0)
,_mini()
,_maxi()
,_doubleValidator(0)
,_intValidator(0)
,animation(0)
,_valueWhenEnteringFocus(0)
,_currentDelta(0)
,_hasChangedSinceLastValidation(false)
,_valueAfterLastValidation(0)
{
    switch (_type) {
        case DOUBLE_SPINBOX:
            _mini.setValue<double>(-DBL_MAX);
            _maxi.setValue<double>(DBL_MAX);
            _doubleValidator = new QDoubleValidator;
            _doubleValidator->setTop(DBL_MAX);
            _doubleValidator->setBottom(-DBL_MAX);
            break;
        case INT_SPINBOX:
            _intValidator = new QIntValidator;
            _mini.setValue<int>(INT_MIN);
            _maxi.setValue<int>(INT_MAX);
            _intValidator->setTop(INT_MAX);
            _intValidator->setBottom(INT_MIN);
            break;
    }
    QObject::connect(this, SIGNAL(returnPressed()), this, SLOT(interpretReturn()));
    setValue(0);
    setMaximumWidth(50);
    setMinimumWidth(35);
    decimals(_decimals);
}
SpinBox::~SpinBox(){
    switch (_type) {
        case DOUBLE_SPINBOX:
            delete _doubleValidator;
            break;
        case INT_SPINBOX:
            delete _intValidator;
            break;
    }
}
void SpinBox::setValue(double d){
    clear();
    QString str;
    switch (_type) {
        case DOUBLE_SPINBOX:
            str.setNum(d,'f',_decimals);
            break;
        case INT_SPINBOX:
            str.setNum((int)d);
            break;
    }
    insert(str);
    home(false);
    _hasChangedSinceLastValidation = false;
    _valueAfterLastValidation = value();
}
void SpinBox::interpretReturn(){
    if (validateText()) {
        emit valueChanged(value());
    }
}

void SpinBox::mousePressEvent(QMouseEvent* e){
    LineEdit::mousePressEvent(e);
}

QString SpinBox::setNum(double cur)
{
    switch (_type) {
        case INT_SPINBOX:
            return QString().setNum((int)cur);
        case DOUBLE_SPINBOX:
        default:
            return QString().setNum(cur,'f',_decimals);
    }
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
        _currentDelta += e->delta();
        double inc = _currentDelta * _increment / 120.;
        switch (_type) {
            case DOUBLE_SPINBOX:
                maxiD = _maxi.toDouble();
                miniD = _mini.toDouble();
                cur += inc;
                _currentDelta = 0;
                break;
            case INT_SPINBOX:
                maxiD = _maxi.toInt();
                miniD = _mini.toInt();
                cur += (int)inc;
                _currentDelta -= ((int)inc) * 120. / _increment;
                assert(std::abs(_currentDelta) < 120);
                break;
        }
        cur = std::max(miniD, std::min(cur,maxiD));
        setValue(cur);
        emit valueChanged(cur);
    }
}

void SpinBox::focusInEvent(QFocusEvent* /*event*/){
    _valueWhenEnteringFocus = text().toDouble();
}

void SpinBox::focusOutEvent(QFocusEvent * /*event*/){
    double newValue = text().toDouble();
    if(newValue != _valueWhenEnteringFocus){
        if(validateText()) {
            emit valueChanged(value());
        }
    }
}

void SpinBox::keyPressEvent(QKeyEvent *e){
    if(isEnabled() && !isReadOnly()){
        bool ok;
        double cur= text().toDouble(&ok);
        double maxiD,miniD;
        switch (_type) {
            case INT_SPINBOX:
                maxiD = _maxi.toInt();
                miniD = _mini.toInt();
                break;
            case DOUBLE_SPINBOX:
            default:
                maxiD = _maxi.toDouble();
                miniD = _mini.toDouble();
                break;
        }
        if(e->key() == Qt::Key_Up){
            clear();

            if(cur+_increment <= maxiD)
                cur+=_increment;
            if(cur < miniD || cur > maxiD)
                return;
            setValue(cur);
            emit valueChanged(cur);
        }else if(e->key() == Qt::Key_Down){
            clear();
            if(cur-_increment >= miniD)
                cur-=_increment;
            if(cur < miniD || cur > maxiD)
                return;
            setValue(cur);
            emit valueChanged(cur);
        } else {
            _hasChangedSinceLastValidation = true;
            QLineEdit::keyPressEvent(e);
        }
    }
}

bool SpinBox::validateText() {
    
    if (!_hasChangedSinceLastValidation) {
        return true;
    }
    
    double maxiD,miniD;
    switch (_type) {
        case INT_SPINBOX:
            maxiD = _maxi.toInt();
            miniD = _mini.toInt();
            break;
        case DOUBLE_SPINBOX:
        default:
            maxiD = _maxi.toDouble();
            miniD = _mini.toDouble();
            break;
    }

    switch (_type) {
        case DOUBLE_SPINBOX: {
            QString txt = text();
            int tmp;
            QValidator::State st = _doubleValidator->validate(txt,tmp);
            if(st == QValidator::Invalid || txt.toDouble() < miniD || txt.toDouble() > maxiD){
                setValue(_valueAfterLastValidation);
                return false;
            } else {
                setValue(value());
                return true;
            }
        } break;
        case INT_SPINBOX: {
            QString txt = text();
            int tmp;
            QValidator::State st = _intValidator->validate(txt,tmp);
            if(st == QValidator::Invalid || txt.toDouble() < miniD || txt.toDouble() > maxiD){
                setValue(_valueAfterLastValidation);
                return false;
            } else {
                setValue(value());
                return true;
            }

        } break;
    }
    return false;
}

void SpinBox::decimals(int d){
    _decimals=d;
    setMaxLength(_decimals+3);
    if(_type == DOUBLE_SPINBOX){
        _doubleValidator->setDecimals(d);
    }
}

void SpinBox::setMaximum(double t) {
    switch (_type) {
        case DOUBLE_SPINBOX:
            _maxi.setValue<double>(t);
            _doubleValidator->setTop(t);
            break;
        case INT_SPINBOX:
            _maxi.setValue<int>((int)t);
            _intValidator->setTop(t);
            break;
    }
}

void SpinBox::setMinimum(double b){
    switch (_type) {
        case DOUBLE_SPINBOX:
            _mini.setValue<double>(b);
            _doubleValidator->setBottom(b);
            break;
        case INT_SPINBOX:
            _mini.setValue<int>(b);
            _intValidator->setBottom(b);
            break;
    }
}

void SpinBox::setAnimation(int i){
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

QMenu* SpinBox::getRightClickMenu() {
    return createStandardContextMenu();
}

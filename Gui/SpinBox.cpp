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

#include "Engine/Variant.h"
#include "Engine/Settings.h"
#include "Engine/AppManager.h"
#include "Global/Macros.h"


struct SpinBoxPrivate
{
    SpinBox::SPINBOX_TYPE type;
    
    /*the precision represents the number of digits after the decimal point.
     For the 'g' and 'G' formats, the precision represents the maximum number
     of significant digits (trailing zeroes are omitted)*/
    int decimals; // for the double spinbox only
    
    double increment;
    int currentDelta; // accumulates the deltas from wheelevents

    Variant mini,maxi;
    QDoubleValidator* doubleValidator;
    QIntValidator* intValidator;
    double valueWhenEnteringFocus;
    bool hasChangedSinceLastValidation;
    double valueAfterLastValidation;
    
    SpinBoxPrivate(SpinBox::SPINBOX_TYPE type)
    : type(type)
    , decimals(2)
    , increment(1.0)
    , currentDelta(0)
    , mini()
    , maxi()
    , doubleValidator(0)
    , intValidator(0)
    , valueWhenEnteringFocus(0)
    , hasChangedSinceLastValidation(false)
    , valueAfterLastValidation(0)
    {
        
    }
    
    void incrementAccordingToPosition(const QString& str,int cursorPos,double& inc);
    
    QString setNum(double cur);
};

SpinBox::SpinBox(QWidget* parent, SPINBOX_TYPE type)
: LineEdit(parent)
, animation(0)
, dirty(false)
, _imp(new SpinBoxPrivate(type))
{
    switch (_imp->type) {
        case DOUBLE_SPINBOX:
            _imp->mini.setValue<double>(-DBL_MAX);
            _imp->maxi.setValue<double>(DBL_MAX);
            _imp->doubleValidator = new QDoubleValidator;
            _imp->doubleValidator->setTop(DBL_MAX);
            _imp->doubleValidator->setBottom(-DBL_MAX);
            break;
        case INT_SPINBOX:
            _imp->intValidator = new QIntValidator;
            _imp->mini.setValue<int>(INT_MIN);
            _imp->maxi.setValue<int>(INT_MAX);
            _imp->intValidator->setTop(INT_MAX);
            _imp->intValidator->setBottom(INT_MIN);
            break;
    }
    QObject::connect(this, SIGNAL(returnPressed()), this, SLOT(interpretReturn()));
    setValue(0);
    setMaximumWidth(50);
    setMinimumWidth(35);
    decimals(_imp->decimals);
}

SpinBox::~SpinBox()
{
    switch (_imp->type) {
        case DOUBLE_SPINBOX:
            delete _imp->doubleValidator;
            break;
        case INT_SPINBOX:
            delete _imp->intValidator;
            break;
    }
}

void
SpinBox::setValue_internal(double d, bool ignoreDecimals)
{
    _imp->valueWhenEnteringFocus = d;
    int pos = cursorPosition();
    clear();
    QString str;
    switch (_imp->type) {
        case DOUBLE_SPINBOX:
            if (!ignoreDecimals) {
                str.setNum(d, 'f', _imp->decimals);
            } else {
                str.setNum(d, 'g', 16);
            }
            break;
        case INT_SPINBOX:
            str.setNum((int)d);
            break;
    }
    assert(!str.isEmpty());
    
    ///Remove trailing 0s by hand...
    int decimalPtPos = str.indexOf(QChar('.'));
    if (decimalPtPos != -1) {
        int i = str.size() - 1;
        while (i > decimalPtPos && str.at(i) == QChar('0')) {
            --i;
        }
        ///let 1 trailing 0
        if (i < str.size() - 1) {
            ++i;
        }
        str = str.left(i + 1);
    }
    
    int i = 0;
    bool skipFirst = false;
    if (str.at(0) == QChar('-')) {
        skipFirst = true;
        ++i;
    }
    while (i < str.size() && i == QChar('0')) {
        ++i;
    }
    str = str.remove(skipFirst ? 1 : 0, i);
    
    insert(str);
    setCursorPosition(pos);
    _imp->hasChangedSinceLastValidation = false;
    _imp->valueAfterLastValidation = value();
}

void
SpinBox::setValue(double d)
{
    setValue_internal(d,false);
}

void
SpinBox::interpretReturn()
{
    if (validateText()) {
        emit valueChanged(value());
    }
}

void
SpinBox::mousePressEvent(QMouseEvent* e)
{
    LineEdit::mousePressEvent(e);
}

QString
SpinBoxPrivate::setNum(double cur)
{
    switch (type) {
        case SpinBox::INT_SPINBOX:
            return QString().setNum((int)cur);
        case SpinBox::DOUBLE_SPINBOX:
        default:
            return QString().setNum(cur,'f',decimals);
    }
}

void
SpinBoxPrivate::incrementAccordingToPosition(const QString& str,int cursorPos,double& inc)
{
    ///Try to find a '.' to handle digits after the decimal point
    int dotPos = str.indexOf('.');
    bool incSet = false;
    
    if (dotPos != -1 && cursorPos >= dotPos) {
        ///Handle digits after decimal point
        ///when cursor == dotPos the cursor is actually on the left of the dot
        if (cursorPos == dotPos) {
            ++cursorPos;
        }
        inc = 1. / std::pow(10.,(double)(cursorPos - dotPos));
        incSet = true;
        
    }
    
    if (!incSet) {
        ///if the character prior to the dot is '-' then increment the first digit after the dot
        if (cursorPos == dotPos - 1 && str.at(cursorPos) == QChar('-')) {
            inc = type == SpinBox::DOUBLE_SPINBOX ? 0.1 : 1; //< we don't want INT_SPINBOX being incremented by lower than 1
        } else {
            if (cursorPos == dotPos -1) {
                inc = 1;
            } else {
                if (dotPos != -1) {
                    inc = std::pow(10.,(double)(dotPos - cursorPos));
                } else {
                    inc = std::pow(10.,(double)str.size() - 1 - cursorPos);
                }
            }
        }
    }
    
   
}

void
SpinBox::wheelEvent(QWheelEvent *e)
{
    setFocus();
    setCursorPosition(cursorPositionAt(e->pos()));
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    if (isEnabled() && !isReadOnly()) {
        QString str = text();
        double cur = str.toDouble();
        double maxiD = 0.;
        double miniD = 0.;
        double inc;
        double old = cur;
        
        bool useCursorPositionIncr = appPTR->getCurrentSettings()->useCursorPositionIncrements();
        
        if (!useCursorPositionIncr) {
            _imp->currentDelta += e->delta();
            inc = _imp->currentDelta * _imp->increment / 120.;
            if (e->modifiers().testFlag(Qt::ShiftModifier)) {
                inc *= 10.;
            }
            if (e->modifiers().testFlag(Qt::ControlModifier)) {
                inc /= 10.;
            }
        } else {
            _imp->incrementAccordingToPosition(str,cursorPosition(),inc);
            if (e->delta() < 0) {
                inc = -inc;
            }
            
        }
        switch (_imp->type) {
            case DOUBLE_SPINBOX:
                maxiD = _imp->maxi.toDouble();
                miniD = _imp->mini.toDouble();
                cur += inc;
                if (!useCursorPositionIncr) {
                    _imp->currentDelta = 0;
                }
                break;
            case INT_SPINBOX:
                maxiD = _imp->maxi.toInt();
                miniD = _imp->mini.toInt();
                cur += (int)inc;
                if (!useCursorPositionIncr) {
                    _imp->currentDelta -= ((int)inc) * 120. / _imp->increment;
                    assert(std::abs(_imp->currentDelta) < 120);
                }
                break;
        }
        cur = std::max(miniD, std::min(cur,maxiD));
        if (cur != old) {
            setValue(cur);
            emit valueChanged(cur);
        }
    }
}

void
SpinBox::focusInEvent(QFocusEvent* event)
{
    _imp->valueWhenEnteringFocus = text().toDouble();
    LineEdit::focusInEvent(event);
}

void
SpinBox::focusOutEvent(QFocusEvent * event)
{
    double newValue = text().toDouble();
    if (newValue != _imp->valueWhenEnteringFocus) {
        if (validateText()) {
            emit valueChanged(value());
        }
    }
    LineEdit::focusOutEvent(event);

}

void
SpinBox::keyPressEvent(QKeyEvent *e)
{
    if (isEnabled() && !isReadOnly()) {
        bool ok;
        double cur = text().toDouble(&ok);
        double old = cur;
        double maxiD,miniD;
        switch (_imp->type) {
            case INT_SPINBOX:
                maxiD = _imp->maxi.toInt();
                miniD = _imp->mini.toInt();
                break;
            case DOUBLE_SPINBOX:
            default:
                maxiD = _imp->maxi.toDouble();
                miniD = _imp->mini.toDouble();
                break;
        }
        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) {
            bool useCursorPositionIncr = appPTR->getCurrentSettings()->useCursorPositionIncrements();
            double inc;
            if (!useCursorPositionIncr) {
                inc = _imp->increment;
                if (e->modifiers().testFlag(Qt::ShiftModifier)) {
                    inc *= 10.;
                }
                if (e->modifiers().testFlag(Qt::ControlModifier)) {
                    inc /= 10.;
                }
            } else {
                int cursorPos = cursorPosition();
                QString txt = text();
                _imp->incrementAccordingToPosition(txt, cursorPos, inc);
            }
            if (e->key() == Qt::Key_Up) {
                if (cur + inc <= maxiD) {
                    cur += inc;
                }
            } else {
                if (cur - inc >= miniD) {
                    cur -= inc;
                }
            }
            if (cur < miniD || cur > maxiD) {
                return;
            }

            if (cur != old) {
                setValue(cur);
                emit valueChanged(cur);
            }
        } else {
            _imp->hasChangedSinceLastValidation = true;
            QLineEdit::keyPressEvent(e);
        }
    }
}

bool
SpinBox::validateText()
{
    if (!_imp->hasChangedSinceLastValidation) {
        return true;
    }
    
    double maxiD,miniD;
    switch (_imp->type) {
        case INT_SPINBOX:
            maxiD = _imp->maxi.toInt();
            miniD = _imp->mini.toInt();
            break;
        case DOUBLE_SPINBOX:
        default:
            maxiD = _imp->maxi.toDouble();
            miniD = _imp->mini.toDouble();
            break;
    }

    switch (_imp->type) {
        case DOUBLE_SPINBOX: {
            QString txt = text();
            int tmp;
            QValidator::State st = _imp->doubleValidator->validate(txt,tmp);
            double val = txt.toDouble();
            if(st == QValidator::Invalid || val < miniD || val > maxiD){
                setValue_internal(_imp->valueAfterLastValidation, true);
                return false;
            } else {
                setValue_internal(val, true);
                return true;
            }
        } break;
        case INT_SPINBOX: {
            QString txt = text();
            int tmp;
            QValidator::State st = _imp->intValidator->validate(txt,tmp);
            int val = txt.toInt();
            if(st == QValidator::Invalid || val < miniD || val > maxiD){
                setValue(_imp->valueAfterLastValidation);
                return false;
            } else {
                setValue(val);
                return true;
            }

        } break;
    }
    return false;
}

void
SpinBox::decimals(int d)
{
    _imp->decimals = d;
}

void
SpinBox::setMaximum(double t)
{
    switch (_imp->type) {
        case DOUBLE_SPINBOX:
            _imp->maxi.setValue<double>(t);
            _imp->doubleValidator->setTop(t);
            break;
        case INT_SPINBOX:
            _imp->maxi.setValue<int>((int)t);
            _imp->intValidator->setTop(t);
            break;
    }
}

void
SpinBox::setMinimum(double b)
{
    switch (_imp->type) {
        case DOUBLE_SPINBOX:
            _imp->mini.setValue<double>(b);
            _imp->doubleValidator->setBottom(b);
            break;
        case INT_SPINBOX:
            _imp->mini.setValue<int>(b);
            _imp->intValidator->setBottom(b);
            break;
    }
}

void
SpinBox::setIncrement(double d)
{
#ifdef OLD_SPINBOX_INCREMENT
    _imp->increment = d;
#else
    (void)d;
#endif
}

void
SpinBox::setAnimation(int i)
{
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void
SpinBox::setDirty(bool d)
{
    dirty = d;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

QMenu*
SpinBox::getRightClickMenu()
{
    return createStandardContextMenu();
}

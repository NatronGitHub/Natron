/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "SpinBox.h"

#include <cfloat>
#include <cmath>
#include <algorithm> // min, max
#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QWheelEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtGui/QDoubleValidator>
#include <QtGui/QIntValidator>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QPainter>

#include "Engine/Variant.h"
#include "Engine/Settings.h"
#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"

#include "Global/Macros.h"

#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/SpinBoxValidator.h"
#include "Gui/GuiApplicationManager.h"

#define SPINBOX_MAX_WIDTH 50
#define SPINBOX_MIN_WIDTH 35

NATRON_NAMESPACE_ENTER;

struct SpinBoxPrivate
{
    SpinBox::SpinBoxTypeEnum type;
    
    /*the precision represents the number of digits after the decimal point.
     For the 'g' and 'G' formats, the precision represents the maximum number
     of significant digits (trailing zeroes are omitted)*/
    int decimals; // for the double spinbox only
    double increment;
    int currentDelta; // accumulates the deltas from wheelevents
    Variant mini,maxi;
    QDoubleValidator* doubleValidator;
    QIntValidator* intValidator;
    QString valueWhenEnteringFocus;
    bool hasChangedSinceLastValidation;
    double valueAfterLastValidation;
    bool valueInitialized; //< false when setValue has never been called yet.
    
    bool useLineColor;
    QColor lineColor;
    
    SpinBoxValidator* customValidator;
    
    SpinBoxPrivate(SpinBox::SpinBoxTypeEnum type)
    : type(type)
    , decimals(2)
    , increment(1.0)
    , currentDelta(0)
    , mini()
    , maxi()
    , doubleValidator(0)
    , intValidator(0)
    , valueWhenEnteringFocus()
    , hasChangedSinceLastValidation(false)
    , valueAfterLastValidation(0)
    , valueInitialized(false)
    , useLineColor(false)
    , lineColor(Qt::black)
    , customValidator(0)
    {
    }
    
    QString setNum(double cur);
};

SpinBox::SpinBox(QWidget* parent,
                 SpinBoxTypeEnum type)
: LineEdit(parent)
, animation(0)
, dirty(false)
, _imp( new SpinBoxPrivate(type) )
{
    QObject::connect( this, SIGNAL( returnPressed() ), this, SLOT( interpretReturn() ) );
    setValue(0);
    setMaximumWidth(TO_DPIX(SPINBOX_MAX_WIDTH));
    setMinimumWidth(TO_DPIX(SPINBOX_MIN_WIDTH));
    setFocusPolicy(Qt::WheelFocus); // mouse wheel gives focus too - see also SpinBox::focusInEvent()
    decimals(_imp->decimals);

    setType(type);
}

SpinBox::~SpinBox()
{
    switch (_imp->type) {
        case eSpinBoxTypeDouble:
            delete _imp->doubleValidator;
            break;
        case eSpinBoxTypeInt:
            delete _imp->intValidator;
            break;
    }
    delete _imp->customValidator;
}

void
SpinBox::setType(SpinBoxTypeEnum type)
{
    _imp->type = type;
    delete _imp->doubleValidator;
    _imp->doubleValidator = 0;
    delete _imp->intValidator;
    _imp->intValidator = 0;
    switch (_imp->type) {
        case eSpinBoxTypeDouble:
            _imp->mini.setValue<double>(-DBL_MAX);
            _imp->maxi.setValue<double>(DBL_MAX);
            _imp->doubleValidator = new QDoubleValidator;
            _imp->doubleValidator->setTop(DBL_MAX);
            _imp->doubleValidator->setBottom(-DBL_MAX);
            setValue_internal(value(),true);
            break;
        case eSpinBoxTypeInt:
            _imp->intValidator = new QIntValidator;
            _imp->mini.setValue<int>(INT_MIN);
            _imp->maxi.setValue<int>(INT_MAX);
            _imp->intValidator->setTop(INT_MAX);
            _imp->intValidator->setBottom(INT_MIN);
            setValue_internal((int)std::floor(value() + 0.5),true);

            break;
    }
}

void
SpinBox::setValue_internal(double d,
                           bool reformat)
{
    if ( ( d == text().toDouble() ) && !reformat && _imp->valueInitialized ) {
        // the value is already OK
        return;
    }

    int pos = cursorPosition();
    QString str;
    switch (_imp->type) {
        case eSpinBoxTypeDouble: {
            str.setNum(d, 'f', _imp->decimals);
            double toDouble = str.toDouble();
            if (d != toDouble) {
                str.setNum(d, 'g', 15);
            }
        }   break;
        case eSpinBoxTypeInt:
            str.setNum( (int)d );
            break;
    }
    assert( !str.isEmpty() );
    
    ///Remove trailing 0s by hand...
    int decimalPtPos = str.indexOf('.');
    if (decimalPtPos != -1) {
        int i = str.size() - 1;
        while (i > decimalPtPos && str.at(i) == '0') {
            --i;
        }
        ///let 1 trailing 0
        if (i < str.size() - 1) {
            ++i;
        }
        str = str.left(i + 1);
    }
    
    // The following removes leading zeroes, but this is not necessary
    /*
     int i = 0;
     bool skipFirst = false;
     if (str.at(0) == '-') {
     skipFirst = true;
     ++i;
     }
     while (i < str.size() && i == '0') {
     ++i;
     }
     if (i > int(skipFirst)) {
     str = str.remove(int(skipFirst), i - int(skipFirst));
     }
     */
    _imp->valueWhenEnteringFocus = str;
    setText(str, pos);
    _imp->valueInitialized = true;
}

void
SpinBox::setText(const QString &str,
                 int cursorPos)
{
    QLineEdit::setText(str);
    
    //qDebug() << "text:" << str << " cursorpos: " << cursorPos;
    setCursorPosition(cursorPos);
    _imp->hasChangedSinceLastValidation = false;
    _imp->valueAfterLastValidation = value();
}

double
SpinBox::getLastValidValueBeforeValidation() const
{
    return _imp->valueAfterLastValidation;
}

void
SpinBox::setValue(double d)
{
    setValue_internal(d, false);
}

void
SpinBox::setValue(int d)
{
    setValue( (double)d );
}

void
SpinBox::interpretReturn()
{
    if ( validateText() ) {
        //setValue_internal(text().toDouble(), true, true); // force a reformat
        Q_EMIT valueChanged( value() );
    }
}



QString
SpinBoxPrivate::setNum(double cur)
{
    switch (type) {
        case SpinBox::eSpinBoxTypeInt:
            
            return QString().setNum( (int)cur );
        case SpinBox::eSpinBoxTypeDouble:
        default:
            
            return QString().setNum(cur,'f',decimals);
    }
}

// common function for wheel up/down and key up/down
// delta is in wheel units: a delta of 120 means to increment by 1
void
SpinBox::increment(int delta,
                   int shift) // shift = 1 means to increment * 10, shift = -1 means / 10
{
    bool ok;
    QString str = text();
    //qDebug() << "increment from " << str;
    const double oldVal = str.toDouble(&ok);
    
    if (!ok) {
        // Not a valid double value, don't do anything
        return;
    }
    
    bool useCursorPositionIncr = appPTR->getCurrentSettings()->useCursorPositionIncrements();
    
    // First, treat the standard case: use the Knob increment
    if (!useCursorPositionIncr) {
        double val = oldVal;
        _imp->currentDelta += delta;
        double inc = std::pow(10., shift) * _imp->currentDelta * _imp->increment / 120.;
        double maxiD = 0.;
        double miniD = 0.;
        switch (_imp->type) {
            case eSpinBoxTypeDouble: {
                maxiD = _imp->maxi.toDouble();
                miniD = _imp->mini.toDouble();
                val += inc;
                _imp->currentDelta = 0;
                break;
            }
            case eSpinBoxTypeInt: {
                maxiD = _imp->maxi.toInt();
                miniD = _imp->mini.toInt();
                val += (int)inc;     // round towards zero
                // Update the current delta, which contains the accumulated error
                _imp->currentDelta -= ( (int)inc ) * 120. / _imp->increment;
                assert(std::abs(_imp->currentDelta) < 120);
                break;
            }
        }
        val = std::max( miniD, std::min(val, maxiD) );
        if (val != oldVal) {
            setValue(val);
            Q_EMIT valueChanged(val);
        }
        
        return;
    }
    
    // From here on, we treat the positin-based increment.
    
    if ( (str.indexOf('e') != -1) || (str.indexOf('E') != -1) ) {
        // Sorry, we don't handle numbers with an exponent, although these are valid doubles
        return;
    }
    
    _imp->currentDelta += delta;
    int inc_int = _imp->currentDelta / 120; // the number of integert increments
    // Update the current delta, which contains the accumulated error
    _imp->currentDelta -= inc_int * 120;
    
    if (inc_int == 0) {
        // Nothing is changed, just return
        return;
    }
    
    // Within the value, we modify:
    // - if there is no selection, the first digit right after the cursor (or if it is an int and the cursor is at the end, the last digit)
    // - if there is a selection, the first digit after the start of the selection
    int len = str.size(); // used for chopping spurious characters
    if (len <= 0) {
        return; // should never happen
    }
    // The position in str of the digit to modify in str() (may be equal to str.size())
    int pos = ( hasSelectedText() ? selectionStart() : cursorPosition() );
    //if (pos == len) { // select the last character?
    //    pos = len - 1;
    //}
    // The position of the decimal dot
    int dot = str.indexOf('.');
    if (dot == -1) {
        dot = str.size();
    }
    
    // Now, chop trailing and leading whitespace (and update len, pos and dot)
    
    // Leading whitespace
    while ( len > 0 && str[0].isSpace() ) {
        str.remove(0, 1);
        --len;
        if (pos > 0) {
            --pos;
        }
        --dot;
        assert(dot >= 0);
        assert(len > 0);
    }
    // Trailing whitespace
    while ( len > 0 && str[len - 1].isSpace() ) {
        str.remove(len - 1, 1);
        --len;
        if (pos > len) {
            --pos;
        }
        if (dot > len) {
            --dot;
        }
        assert(len > 0);
    }
    assert( oldVal == str.toDouble() ); // check that the value hasn't changed due to whitespace manipulation
    
    // On int types, there should not be any dot
    if ( (_imp->type == eSpinBoxTypeInt) && (len > dot) ) {
        // Remove anything after the dot, including the dot
        str.resize(dot);
        len = dot;
    }
    
    // Adjust pos so that it doesn't point to a dot or a sign
    assert( 0 <= pos && pos <= str.size() );
    while ( pos < str.size() &&
           (pos == dot || str[pos] == '+' || str[pos] == '-') ) {
        ++pos;
    }
    assert(len >= pos);
    
    // Set the shift (may have to be done twice due to the dot)
    pos -= shift;
    if (pos == dot) {
        pos -= shift;
    }
    
    // Now, add leading and trailing zeroes so that pos is a valid digit position
    // (beware of the sign!)
    // Trailing zeroes:
    // (No trailing zeroes on int, of course)
    assert( len == str.size() );
    if ( (_imp->type == eSpinBoxTypeInt) && (pos >= len) ) {
        // If this is an int and we are beyond the last position, change the last digit
        pos = len - 1;
        // also reset the shift if it was negative
        if (shift < 0) {
            shift = 0;
        }
    }
    while ( pos >= str.size() ) {
        assert(_imp->type == eSpinBoxTypeDouble);
        // Add trailing zero, maybe preceded by a dot
        if (pos == dot) {
            str.append('.');
            ++pos; // increment pos, because we just added a '.', and next iteration will add a '0'
            ++len;
        } else {
            assert(pos > dot);
            str.append('0');
            ++len;
        }
        assert( pos >= (str.size() - 1) );
    }
    // Leading zeroes:
    bool hasSign = (str[0] == '-' || str[0] == '+');
    while ( pos < 0 || ( pos == 0 && (str[0] == '-' || str[0] == '+') ) ) {
        // Add leading zero
        str.insert(hasSign ? 1 : 0, '0');
        ++pos;
        ++dot;
        ++len;
    }
    assert( len == str.size() );
    assert( 0 <= pos && pos < str.size() && str[pos].isDigit() );
    
    QString noDotStr = str;
    int noDotLen = len;
    if (dot != len) {
        // Remove the dot
        noDotStr.remove(dot, 1);
        --noDotLen;
    }
    assert( (_imp->type == eSpinBoxTypeInt && noDotLen == dot) || noDotLen >= dot );
    double val = oldVal; // The value, as a double
    if (noDotLen > 16 && 16 >= dot) {
        // don't handle more than 16 significant digits (this causes over/underflows in the following)
        assert(noDotLen == noDotStr.size());
        noDotLen = 16;
        noDotStr.resize(noDotLen);
    }
    qlonglong llval = noDotStr.toLongLong(&ok); // The value, as a long long int
    if (!ok) {
        // Not a valid long long value, don't do anything
        return;
    }
    int llpowerOfTen = dot - noDotLen; // llval must be post-multiplied by this power of ten
    assert(llpowerOfTen <= 0);
    // check that val and llval*10^llPowerOfTen are close enough (relative error should be less than 1e-8)
    assert(std::abs(val * std::pow(10.,-llpowerOfTen) - llval) / std::max(qlonglong(1),std::abs(llval)) < 1e-8);
    
    
    // If pos is at the end
    if ( pos == str.size() ) {
        switch (_imp->type) {
            case eSpinBoxTypeDouble:
                if ( dot == str.size() ) {
                    str += ".0";
                    len += 2;
                    ++pos;
                } else {
                    str += "0";
                    ++len;
                }
                break;
            case eSpinBoxTypeInt:
                // take the character before
                --pos;
                break;
        }
    }
    
    // Compute the full value of the increment
    assert( len == str.size() );
    assert(pos != dot);
    assert( 0 <= pos && pos < len && str[pos].isDigit() );
    
    int powerOfTen = dot - pos - (pos < dot); // the power of ten
    assert( (_imp->type == eSpinBoxTypeDouble) || ( powerOfTen >= 0 && dot == str.size() ) );

    if (powerOfTen - llpowerOfTen > 16) {
        // too many digits to handle, don't do anything
        // (may overflow when adjusting llval)
        return;
    }

    double inc = inc_int * std::pow(10., (double)powerOfTen);
    
    // Check that we are within the authorized range
    double maxiD,miniD;
    switch (_imp->type) {
        case eSpinBoxTypeInt:
            maxiD = _imp->maxi.toInt();
            miniD = _imp->mini.toInt();
            break;
        case eSpinBoxTypeDouble:
        default:
            maxiD = _imp->maxi.toDouble();
            miniD = _imp->mini.toDouble();
            break;
    }
    val += inc;
    if ( (val < miniD) || (maxiD < val) ) {
        // out of the authorized range, don't do anything
        return;
    }

    // Adjust llval so that the increment becomes an int, and avoid rounding errors
    if (powerOfTen >= llpowerOfTen) {
        llval += inc_int * std::pow(10., powerOfTen - llpowerOfTen);
    } else {
        llval *= std::pow(10., llpowerOfTen - powerOfTen);
        llpowerOfTen -= llpowerOfTen - powerOfTen;
        llval += inc_int;
    }
    // check that val and llval*10^llPowerOfTen are still close enough (relative error should be less than 1e-8)
    assert(std::abs(val * std::pow(10.,-llpowerOfTen) - llval) / std::max(qlonglong(1),std::abs(llval)) < 1e-8);

    QString newStr;
    newStr.setNum(llval);
    bool newStrHasSign = newStr[0] == '+' || newStr[0] == '-';
    // the position of the decimal dot
    int newDot = newStr.size() + llpowerOfTen;
    // add leading zeroes if newDot is not a valid position (beware of sign!)
    while ( newDot <= int(newStrHasSign) ) {
        newStr.insert(int(newStrHasSign), '0');
        ++newDot;
    }
    assert( 0 <= newDot && newDot <= newStr.size() );
    assert( newDot == newStr.size() || newStr[newDot].isDigit() );
    if ( newDot != newStr.size() ) {
        assert(_imp->type == eSpinBoxTypeDouble);
        newStr.insert(newDot, '.');
    }
    // Check that the backed string is close to the wanted value (relative error should be less than 1e-8)
    assert( (newStr.toDouble() - val) / std::max(1e-8,std::abs(val)) < 1e-8 );
    // The new cursor position
    int newPos = newDot + (pos - dot);
    // Remove the shift (may have to be done twice due to the dot)
    newPos += shift;
    if (newPos == newDot) {
        // adjust newPos
        newPos += shift;
    }
    
    assert( 0 <= newDot && newDot <= newStr.size() );
    
    // Now, add leading and trailing zeroes so that newPos is a valid digit position
    // (beware of the sign!)
    // Trailing zeroes:
    while ( newPos >= newStr.size() ) {
        assert(_imp->type == eSpinBoxTypeDouble);
        // Add trailing zero, maybe preceded by a dot
        if (newPos == newDot) {
            newStr.append('.');
        } else {
            assert(newPos > newDot);
            newStr.append('0');
        }
        assert( newPos >= (newStr.size() - 1) );
    }
    // Leading zeroes:
    bool newHasSign = (newStr[0] == '-' || newStr[0] == '+');
    while ( newPos < 0 || ( newPos == 0 && (newStr[0] == '-' || newStr[0] == '+') ) ) {
        // add leading zero
        newStr.insert(newHasSign ? 1 : 0, '0');
        ++newPos;
        ++newDot;
    }
    assert( 0 <= newPos && newPos < newStr.size() && newStr[newPos].isDigit() );
    
    // Set the text and cursor position
    //qDebug() << "increment setting text to " << newStr;
    setText(newStr, newPos);
    // Set the selection
    assert( newPos + 1 <= newStr.size() );
    setSelection(newPos + 1, -1);
    Q_EMIT valueChanged( value() );
} // increment

void
SpinBox::wheelEvent(QWheelEvent* e)
{
    if ( (e->orientation() != Qt::Vertical) ||
        ( e->delta() == 0) ||
        !isEnabled() ||
        isReadOnly() ||
        !hasFocus() ) { // wheel is only effective when the widget has focus (click it first, then wheel)
        return;
    }
    int delta = e->delta();
    int shift = 0;
    if ( modCASIsShift(e) ) {
        shift = 1;
    }
    if ( modCASIsControl(e) ) {
        shift = -1;
    }
    increment(delta, shift);
}

void
SpinBox::focusInEvent(QFocusEvent* e)
{
    //qDebug() << "focusin";
    
    // don't give focus if this is a wheelEvent
    if (e->reason() == Qt::MouseFocusReason) {
        Qt::MouseButtons buttons = QApplication::mouseButtons();
        if ( !(buttons & Qt::LeftButton) && !(buttons & Qt::RightButton)) {
            this->clearFocus();
            
            return;
        }
    } else if (e->reason() == Qt::OtherFocusReason || e->reason() == Qt::TabFocusReason) {
        //If the user tabbed into it or hovered it, select all text
        selectAll();
    }
    _imp->valueWhenEnteringFocus = text();
    LineEdit::focusInEvent(e);
}




void
SpinBox::focusOutEvent(QFocusEvent* e)
{
    //qDebug() << "focusout";
    QString str = text();
    if (str != _imp->valueWhenEnteringFocus) {
        if ( validateText() ) {
            //setValue_internal(text().toDouble(), true, true); // force a reformat
            bool ok;
            QString newValueStr = text();
            double newValue = newValueStr.toDouble(&ok);
            
            if (!ok) {
                newValueStr = _imp->valueWhenEnteringFocus;
                newValue = newValueStr.toDouble(&ok);
                assert(ok);
            }
            QLineEdit::setText(newValueStr);
            Q_EMIT valueChanged(newValue);
        }
    } else {
        bool ok;
        (void)str.toDouble(&ok);
        if (!ok) {
            QLineEdit::setText(_imp->valueWhenEnteringFocus);
        }
    }
    LineEdit::focusOutEvent(e);
}

void
SpinBox::keyPressEvent(QKeyEvent* e)
{
    //qDebug() << "keypress";
    if ( isEnabled() && !isReadOnly() ) {
        if ( (e->key() == Qt::Key_Up) || (e->key() == Qt::Key_Down) ) {
            int delta = (e->key() == Qt::Key_Up) ? 120 : -120;
            int shift = 0;
            if ( modCASIsShift(e) ) {
                shift = 1;
            }
            if ( modCASIsControl(e) ) {
                shift = -1;
            }
            increment(delta, shift);
        } else {
            _imp->valueWhenEnteringFocus = value();
            _imp->hasChangedSinceLastValidation = true;
            QLineEdit::keyPressEvent(e);
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                ///Return and enter emit editingFinished() in parent implementation but do not accept the shortcut either
                e->accept();
            }
        }
    } else {
        QLineEdit::keyPressEvent(e);
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            ///Return and enter emit editingFinished() in parent implementation but do not accept the shortcut either
            e->accept();
        }
    }
}

void
SpinBox::setValidator(SpinBoxValidator* validator)
{
    _imp->customValidator = validator;
}

bool
SpinBox::validateWithCustomValidator(const QString& txt)
{
    if (_imp->customValidator) {
        double valueToDisplay;
        if (_imp->customValidator->validateInput(txt, &valueToDisplay)) {
            setValue_internal(_imp->type == eSpinBoxTypeDouble ? valueToDisplay : (int)valueToDisplay, true);
            return true;
        }
    }
    setValue_internal(_imp->valueAfterLastValidation, true);
    return false;
}

bool
SpinBox::validateInternal()
{
    
    QString txt = text();
    const QValidator* validator = 0;
    if (_imp->type == eSpinBoxTypeDouble) {
        validator = _imp->doubleValidator;
    } else {
        assert(_imp->type == eSpinBoxTypeInt);
        validator = _imp->intValidator;
    }
    assert(validator);
    int tmp;
    QValidator::State st = QValidator::Invalid;
    if (!txt.isEmpty()) {
        st = validator->validate(txt,tmp);
    }
    double val;
    double maxiD,miniD;
    if (_imp->type == eSpinBoxTypeDouble) {
        val = txt.toDouble();
        maxiD = _imp->maxi.toDouble();
        miniD = _imp->mini.toDouble();

    } else {
        assert(_imp->type == eSpinBoxTypeInt);
        val = (double)txt.toInt();
        maxiD = _imp->maxi.toInt();
        miniD = _imp->mini.toInt();
    }
    if (st == QValidator::Invalid) {
        return validateWithCustomValidator(txt);
    } else if ((val < miniD) ||
               (val > maxiD)) {
        setValue_internal(_imp->valueAfterLastValidation, true);
        return false;
    } else {
        setValue_internal(val, false);
        return true;
    }

}

bool
SpinBox::validateText()
{
    if (!_imp->hasChangedSinceLastValidation) {
        return true;
    }
    
    
    return validateInternal();
} // validateText

void
SpinBox::decimals(int d)
{
    _imp->decimals = d;
}

void
SpinBox::setMaximum(double t)
{
    switch (_imp->type) {
        case eSpinBoxTypeDouble:
            _imp->maxi.setValue<double>(t);
            _imp->doubleValidator->setTop(t);
            break;
        case eSpinBoxTypeInt:
            _imp->maxi.setValue<int>( (int)t );
            _imp->intValidator->setTop(t);
            break;
    }
}

void
SpinBox::setMinimum(double b)
{
    switch (_imp->type) {
        case eSpinBoxTypeDouble:
            _imp->mini.setValue<double>(b);
            _imp->doubleValidator->setBottom(b);
            break;
        case eSpinBoxTypeInt:
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
    Q_UNUSED(d);
#endif
}

void
SpinBox::setAnimation(int i)
{
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
SpinBox::setDirty(bool d)
{
    dirty = d;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

QMenu*
SpinBox::getRightClickMenu()
{
    QMenu* menu =  createStandardContextMenu();
    menu->setFont(QApplication::font()); // necessary
    return menu;
}

void
SpinBox::paintEvent(QPaintEvent* e)
{
    LineEdit::paintEvent(e);
    
    if (_imp->useLineColor) {
        QPainter p(this);
        p.setPen(_imp->lineColor);
        int h = height() - 1;
        p.drawLine(0, h - 1, width() - 1, h - 1);
    }
}

void
SpinBox::setUseLineColor(bool use, const QColor& color)
{
    _imp->useLineColor = use;
    _imp->lineColor = color;
    update();
}

void
KnobSpinBox::enterEvent(QEvent* e)
{
    mouseEnter(e);
    SpinBox::enterEvent(e);
}

void
KnobSpinBox::leaveEvent(QEvent* e)
{
    mouseLeave(e);
    SpinBox::leaveEvent(e);
}

void
KnobSpinBox::keyPressEvent(QKeyEvent* e)
{
    keyPress(e);
    SpinBox::keyPressEvent(e);
}

void
KnobSpinBox::keyReleaseEvent(QKeyEvent* e)
{
    keyRelease(e);
    SpinBox::keyReleaseEvent(e);
}

void
KnobSpinBox::mousePressEvent(QMouseEvent* e)
{
    if (!mousePress(e)) {
        SpinBox::mousePressEvent(e);
    }
}

void
KnobSpinBox::mouseMoveEvent(QMouseEvent* e)
{
    if (!mouseMove(e)) {
        SpinBox::mouseMoveEvent(e);
    }
}

void
KnobSpinBox::mouseReleaseEvent(QMouseEvent* e)
{
    mouseRelease(e);
    SpinBox::mouseReleaseEvent(e);

}

void
KnobSpinBox::dragEnterEvent(QDragEnterEvent* e)
{
    if (!dragEnter(e)) {
        SpinBox::dragEnterEvent(e);
    }
}

void
KnobSpinBox::dragMoveEvent(QDragMoveEvent* e)
{
    if (!dragMove(e)) {
        SpinBox::dragMoveEvent(e);
    }
}
void
KnobSpinBox::dropEvent(QDropEvent* e)
{
    if (!drop(e)) {
        SpinBox::dropEvent(e);
    }
}

void
KnobSpinBox::focusInEvent(QFocusEvent* e)
{
    focusIn();
    SpinBox::focusInEvent(e);
    
    
    //Set the expression so the user can edit it easily
    std::string expr = knob->getKnob()->getExpression(dimension);
    if (expr.empty()) {
        return;
    } else {
        QLineEdit::setText(expr.c_str());
        setCursorPosition(expr.size() - 1);
    }
}

void
KnobSpinBox::focusOutEvent(QFocusEvent* e)
{
    focusOut();
    SpinBox::focusOutEvent(e);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_SpinBox.cpp"

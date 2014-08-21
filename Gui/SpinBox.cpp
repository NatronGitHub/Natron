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
#include <QDebug>

#include "Engine/Variant.h"
#include "Engine/Settings.h"
#include "Engine/AppManager.h"
#include "Global/Macros.h"
#include "Gui/GuiMacros.h"


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
    
    int i = 0;
    bool skipFirst = false;
    if (str.at(0) == '-') {
        skipFirst = true;
        ++i;
    }
    while (i < str.size() && i == '0') {
        ++i;
    }
    str = str.remove(skipFirst ? 1 : 0, i);

    //qDebug() << "setValue_internal setting text to "<<str;
    setText(str, pos);
}

void
SpinBox::setText(const QString &str, int cursorPos)
{
    QLineEdit::setText(str);
    //qDebug() << "text:" << str << " cursorpos: " << cursorPos;
    setCursorPosition(cursorPos);
    _imp->hasChangedSinceLastValidation = false;
    _imp->valueAfterLastValidation = value();
}

void
SpinBox::setValue(double d)
{
    setValue_internal(d,false);
}

void
SpinBox::setValue(int d)
{
    setValue((double)d);
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
    setFocus();
    setCursorPosition(cursorPositionAt(e->pos()));
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

// common function for wheel up/down and key up/down
// delta is in wheel units: a delta of 120 means to increment by 1
void
SpinBox::increment(int delta)
{
    bool ok;
    QString str = text();
    //qDebug() << "increment from " << str;
    const double oldVal = str.toDouble(&ok);
    if (!ok) {
        // not a valid double value, don't do anything
        return;
    }

    bool useCursorPositionIncr = appPTR->getCurrentSettings()->useCursorPositionIncrements();

    if (!useCursorPositionIncr) {
        double val = oldVal;
        _imp->currentDelta += delta;
        double inc = _imp->currentDelta * _imp->increment / 120.;
        double maxiD = 0.;
        double miniD = 0.;
        switch (_imp->type) {
            case DOUBLE_SPINBOX: {
                maxiD = _imp->maxi.toDouble();
                miniD = _imp->mini.toDouble();
                val += inc;
                _imp->currentDelta = 0;
            }   break;
            case INT_SPINBOX: {
                maxiD = _imp->maxi.toInt();
                miniD = _imp->mini.toInt();
                val += (int)inc; // round towards zero
                // update the current delta, which contains the accumulated error
                _imp->currentDelta -= ((int)inc) * 120. / _imp->increment;
                assert(std::abs(_imp->currentDelta) < 120);
            }   break;
        }
        val = std::max(miniD, std::min(val, maxiD));
        if (val != oldVal) {
            setValue(val);
            emit valueChanged(val);
        }
        return;
    }

    if (str.indexOf('e') != -1 || str.indexOf('E') != -1) {
        // sorry, we don't handle numbers with an exponent, although these are valid doubles
        return;
    }

    _imp->currentDelta += delta;
    int inc_int = _imp->currentDelta / 120; // the number of integert increments
    // update the current delta, which contains the accumulated error
    _imp->currentDelta -= inc_int * 120;

    if (inc_int != 0) {
        // we modify:
        // - if there is no selection, the first digit right after the cursor (or if it is an int and the cursor is at the end, the last digit)
        // - if there is a selection, the first digit after the start of the selection
        int len = str.size(); // used for chopping spurious characters
        // the position in str of the digit to modify in str() (may be equal to str.size())
        int pos = hasSelectedText() ? selectionStart() : cursorPosition();
        // the position of the decimal dot
        int dot = str.indexOf('.');
        if (dot == -1) {
            dot = str.size();
        }

        // now, chop trailing and leading whitespace (and update len, pos and dot)

        // leading whitespace
        while (len > 0 && str[0].isSpace()) {
            str.remove(0, 1);
            --len;
            if (pos > 0) {
                --pos;
            }
            --dot;
            assert(dot >= 0);
            assert(len > 0);
        }
        // trailing whitespace
        while (len > 0 && str[len-1].isSpace()) {
            str.remove(len-1, 1);
            --len;
            if (pos > len) {
                --pos;
            }
            if (dot > len) {
                --dot;
            }
            assert(len > 0);
        }
        assert(oldVal == str.toDouble()); // check that the value hasn't changed due to whitespace manipulation

        // on int types, there should not be any dot
        if (_imp->type == INT_SPINBOX && len > dot) {
            // remove anything after the dot, including the dot
            str.resize(dot);
            len = dot;
            //qDebug() << "trimmed dot, text is now "<<str;
        }

        QString noDotStr = str;
        int noDotLen = len;
        if (dot != len) {
            // remove the dot
            noDotStr.remove(dot, 1);
            --noDotLen;
        }
        assert((_imp->type == INT_SPINBOX && noDotLen == dot) || noDotLen >= dot);
        double val = oldVal;

        qlonglong llval = noDotStr.toLongLong();
        int llpowerOfTen = dot - noDotLen; // llval must be post-multiplied by this power of ten
        assert(llpowerOfTen <= 0);
        // check that val and llval*10^llPowerOfTen are close enough
        assert(std::fabs(val * std::pow(10.,-llpowerOfTen) - llval) < 1e-8);

        assert(0 <= pos && pos <= str.size());
        while (pos < str.size() &&
               (pos == dot || str[pos] == '+' || str[pos] == '-')) {
            ++pos;
        }
        assert(len >= pos);

        // if pos is at the end
        if (pos == str.size()) {
            switch (_imp->type) {
                case DOUBLE_SPINBOX:
                    if (dot == str.size()) {
                        str += ".0";
                        len += 2;
                        ++pos;
                    } else {
                        str += "0";
                        ++len;
                    }
                    break;
                case INT_SPINBOX:
                    // take the character before
                    --pos;
                    break;
            }
        }

        // compute the full value of the increment
        assert(pos != dot);
        assert(0 <= pos && pos < str.size() && str[pos].isDigit());

        int powerOfTen = dot - pos - (pos < dot); // the power of ten
        assert((_imp->type == DOUBLE_SPINBOX) || (powerOfTen >= 0 && dot == str.size()));

        double inc = inc_int * std::pow(10., (double)powerOfTen);

        // check that we are within the authorized range
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
        val += inc;
        if (val < miniD || maxiD < val) {
            // out of the authorized range, don't do anything
            return;
        }

        // adjust llval so that the increment becomes an int, and avoid rounding errors
        if (powerOfTen >= llpowerOfTen) {
            llval += inc_int * std::pow(10., powerOfTen - llpowerOfTen);
        } else {
            llval *= std::pow(10., llpowerOfTen - powerOfTen);
            llpowerOfTen -= llpowerOfTen - powerOfTen;
            llval += inc_int;
        }
        // check that val and llval*10^llPowerOfTen are still close enough
        assert(std::fabs(val * std::pow(10.,-llpowerOfTen) - llval) < 1e-8);

        QString newStr;
        newStr.setNum(llval);
        bool newStrHasSign = newStr[0] == '+' || newStr[0] == '-';
        // the position of the decimal dot
        int newDot = newStr.size() + llpowerOfTen;
        // add leading zeroes if newDot is not a valid position (beware of sign!)
        while (newDot <= int(newStrHasSign)) {
            newStr.insert(int(newStrHasSign), '0');
            ++newDot;
        }
        assert(0 <= newDot && newDot <= newStr.size());
        assert(newDot == newStr.size() || newStr[newDot].isDigit());
        if (newDot != newStr.size()) {
            assert(_imp->type == DOUBLE_SPINBOX);
            newStr.insert(newDot, '.');
        }
        // check that the backed string is close to the wanted value
        assert((newStr.toDouble() - val) * std::pow(10.,-llpowerOfTen) < 1e-8);
        // the new cursor position
        int newPos = newDot + (pos - dot);

        assert(0 <= newDot && newDot <= newStr.size());

        // now, add leading and trailing zeroes so that newPos is a valid digit position
        // (beware of the sign!)

        while (newPos >= newStr.size()) {
            assert(_imp->type == DOUBLE_SPINBOX);
            // add trailing zero, maybe preceded by a dot
            if (newPos == newDot) {
                newStr.append('.');
            } else {
                assert(newPos > newDot);
                newStr.append('0');
            }
            assert(newPos >= (newStr.size() - 1));
        }

        while (newPos < 0 || (newPos == 0 && (newStr[0] == '-' || newStr[0] == '+'))) {
            // add leading zero
            bool hasSign = (newStr[0] == '-' || newStr[0] == '+');
            newStr.insert(hasSign ? 1 : 0, '0');
            ++newPos;
            ++newDot;
        }

        assert(0 <= newPos && newPos < newStr.size() && newStr[newPos].isDigit());

        // set the text and cursor position
        //qDebug() << "increment setting text to " << newStr;
        setText(newStr, newPos);
        // set the selection
        assert(newPos+1 <= newStr.size());
        setSelection(newPos + 1, -1);
    } // if (inc_int != 0)
}

void
SpinBox::wheelEvent(QWheelEvent* e)
{
    if (e->orientation() != Qt::Vertical ||
        e->delta() == 0 ||
        !isEnabled() ||
        isReadOnly()) {
        return;
    }
    setFocus();
    int delta = e->delta();
    if (modifierIsShift(e)) {
        delta *= 10;
    }
    if (modifierIsControl(e)) {
        delta /= 10;
    }
    increment(delta);
}

void
SpinBox::focusInEvent(QFocusEvent* e)
{
    //qDebug() << "focusin";
    _imp->valueWhenEnteringFocus = text().toDouble();
    LineEdit::focusInEvent(e);
}

void
SpinBox::focusOutEvent(QFocusEvent* e)
{
    //qDebug() << "focusout";
    double newValue = text().toDouble();
    if (newValue != _imp->valueWhenEnteringFocus) {
        if (validateText()) {
            emit valueChanged(value());
        }
    }
    LineEdit::focusOutEvent(e);
}

void
SpinBox::keyPressEvent(QKeyEvent* e)
{
    //qDebug() << "keypress";
    if (isEnabled() && !isReadOnly()) {
        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) {
            int delta = (e->key() == Qt::Key_Up) ? 120 : -120;
            if (modifierIsShift(e)) {
                delta *= 10;
            }
            if (modifierIsControl(e)) {
                delta /= 10;
            }
            increment(delta);
        } else {
            _imp->hasChangedSinceLastValidation = true;
            QLineEdit::keyPressEvent(e);
        }
    } else {
        QLineEdit::keyPressEvent(e);
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

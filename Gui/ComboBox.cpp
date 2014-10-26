//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Gui/ComboBox.h"

#include <cassert>
#include <algorithm>
#include <QLayout>
#include <QStyle>
#include <QFont>
#include <QStyleOption>
#include <QFontMetrics>
#include <QPainter>
#include <QTextDocument> // for Qt::convertFromPlainText
CLANG_DIAG_OFF(unused-private-field)
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)
CLANG_DIAG_ON(deprecated-register)

#include "Gui/GuiApplicationManager.h"
#include "Gui/MenuWithToolTips.h"
#include "Gui/ClickableLabel.h"
#include "Gui/GuiMacros.h"

#define DROP_DOWN_ICON_SIZE 6

using namespace Natron;

ComboBox::ComboBox(QWidget* parent)
    : QFrame(parent)
    , _readOnly(false)
    , _enabled(true)
    , _animation(0)
    , _clicked(false)
    , _dirty(false)
    , _currentIndex(0)
    , _currentText()
    , _separators()
    , _actions()
    , _menu(0)
    , _sh()
    , _msh()
    , _sizePolicy()
    , _validHints(false)
    , _align(Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs)
{

    setFrameShape(QFrame::Box);

    setCurrentIndex(-1);

    _menu = new MenuWithToolTips(this);

    setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed,QSizePolicy::Label));
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight(NATRON_MEDIUM_BUTTON_SIZE);
}

QSize
ComboBox::sizeForWidth(int w) const
{
    if (minimumWidth() > 0) {
        w = std::max(w,maximumWidth());
    }
    QSize contentsMargin;
    {
        QMargins margins = contentsMargins();
        contentsMargin.setWidth(margins.left() + margins.right());
        contentsMargin.setHeight(margins.bottom() + margins.top());
    }
    QRect br;
    
    QFontMetrics fm = fontMetrics();
    
    Qt::Alignment align = QStyle::visualAlignment(Qt::LeftToRight, QFlag(_align));
    
    int hextra = DROP_DOWN_ICON_SIZE * 2,vextra = 0;
    
    ///Indent of 1 character
    int indent = fm.width('x');
    
    if (indent > 0) {
        if ((align & Qt::AlignLeft) || (align & Qt::AlignRight))
            hextra += indent;
        if ((align & Qt::AlignTop) || (align & Qt::AlignBottom))
            vextra += indent;
    }
    // Turn off center alignment in order to avoid rounding errors for centering,
    // since centering involves a division by 2. At the end, all we want is the size.
    int flags = align & ~(Qt::AlignVCenter | Qt::AlignHCenter);
    
    bool tryWidth = (w < 0) && (align & Qt::TextWordWrap);
    
    if (tryWidth) {
        w = std::min(fm.averageCharWidth() * 80, maximumSize().width());
    } else if (w < 0) {
        w = 2000;
    }
    
    w -= (hextra + contentsMargin.width());
    
    br = fm.boundingRect(0, 0, w ,2000, flags, _currentText);
    
    if (tryWidth && br.height() < 4 * fm.lineSpacing() && br.width() > w / 2) {
        br = fm.boundingRect(0, 0, w / 2, 2000, flags, _currentText);
    }
    
    if (tryWidth && br.height() < 2 * fm.lineSpacing() && br.width() > w / 4) {
        br = fm.boundingRect(0, 0, w / 4, 2000, flags, _currentText);
    }
    
    const QSize contentsSize(br.width() + hextra, br.height() + vextra);
    
    return (contentsSize + contentsMargin).expandedTo(minimumSize());
}

QSize
ComboBox::sizeHint() const
{
    if (_validHints) {
        return minimumSizeHint();
    }
    return _sh;
}

QSize
ComboBox::minimumSizeHint() const
{
    if (_validHints && _sizePolicy == sizePolicy()) {
        return _msh;
    }
    ensurePolished();
    _validHints = true;
    _sh = sizeForWidth(-1);
    
    _msh.rheight() = sizeForWidth(QWIDGETSIZE_MAX).height(); // height for one line
    _msh.rwidth() = sizeForWidth(0).width();
    
    if (_sh.height() < _msh.height()) {
        _msh.rheight() = _sh.height();
    }
    _sizePolicy = sizePolicy();
    return _msh;
}

void
ComboBox::updateLabel()
{
    _validHints = false;
    updateGeometry(); //< force a call to minmumSizeHint
    update();
}

void
ComboBox::changeEvent(QEvent* e)
{
    if(e->type() == QEvent::FontChange || e->type() == QEvent::ApplicationFontChange ||
       e->type() == QEvent::ContentsRectChange) {
        updateLabel();
    }
    QFrame::changeEvent(e);
}


void
ComboBox::resizeEvent(QResizeEvent* e)
{
    updateLabel();
    QFrame::resizeEvent(e);
}

QRectF
ComboBox::layoutRect() const
{
    QRect cr = contentsRect();
    
    Qt::Alignment align = QStyle::visualAlignment(Qt::LeftToRight, QFlag(_align));
    
    int indent = fontMetrics().width('x') / 2;
    if (indent > 0) {
        
        if (align & Qt::AlignLeft)
            cr.setLeft(cr.left() + indent);
        
        if (align & Qt::AlignRight)
            cr.setRight(cr.right() - indent);
        
        if (align & Qt::AlignTop)
            cr.setTop(cr.top() + indent);
        
        if (align & Qt::AlignBottom)
            cr.setBottom(cr.bottom() - indent);
    }
    cr.adjust(0, 0, -DROP_DOWN_ICON_SIZE * 2, 0);
    return cr;
}

void
ComboBox::paintEvent(QPaintEvent* /*e*/)
{
    
    QStyleOption opt;
    opt.initFrom(this);
    
    QPainter p(this);
   
    QRectF bRect = rect();

    {
        ///Now draw the frame

        QColor fillColor;
        if (_clicked || _dirty) {
            fillColor = Qt::black;
        } else {
            switch (_animation) {
                case 0:
                default:
                    fillColor.setRgb(71,71,71);
                    break;
                case 1:
                    fillColor.setRgb(86,117,156);
                    break;
                case 2:
                    fillColor.setRgb(21,97,248);
                    break;
                    
            }
        }
        
        double fw = frameWidth();

        QPen pen;
        if (!hasFocus()) {
            pen.setColor(Qt::black);
        } else {
            pen.setColor(QColor(243,137,0));
            fw = 2;
        }
        p.setPen(pen);
        
    
        QRectF roundedRect = bRect.adjusted(fw / 2., fw / 2., -fw, -fw);
        bRect.adjust(fw, fw, -fw, -fw);
        p.fillRect(bRect, fillColor);
        double roundPixels = 3;
        
        
        QPainterPath path;
        path.addRoundedRect(roundedRect, roundPixels, roundPixels);
        p.drawPath(path);

    }
    
    QColor textColor;
    if (_readOnly) {
        textColor.setRgb(100,100,100);
    } else if (!_enabled) {
        textColor = Qt::black;
    } else {
        textColor.setRgb(200,200,200);
    }
    {
        Qt::Alignment align = QStyle::visualAlignment(Qt::LeftToRight, QFlag(_align));
        
        int flags = align | Qt::TextForceLeftToRight;
        
        ///Draw the text
        QPen pen;
        pen.setColor(textColor);
        p.setPen(pen);
        
        QRectF lr = layoutRect().toAlignedRect();
        p.drawText(lr.toRect(), flags, _currentText);
    }
    
    {
        ///Draw the dropdown icon
        QPainterPath path;
        QPolygonF poly;
        poly.push_back(QPointF(bRect.right() - DROP_DOWN_ICON_SIZE * 3. / 2.,bRect.height() / 2. - DROP_DOWN_ICON_SIZE / 2.));
        poly.push_back(QPointF(bRect.right() - DROP_DOWN_ICON_SIZE / 2.,bRect.height() / 2. - DROP_DOWN_ICON_SIZE / 2.));
        poly.push_back(QPointF(bRect.right() - DROP_DOWN_ICON_SIZE,bRect.height() / 2. + DROP_DOWN_ICON_SIZE / 2.));
        path.addPolygon(poly);
        p.fillPath(path,textColor);
        
    }
}

void
ComboBox::mousePressEvent(QMouseEvent* e)
{
    if ( buttonDownIsLeft(e) && !_readOnly && _enabled ) {
        _clicked = true;
        createMenu();
        repaint();
        QFrame::mousePressEvent(e);
    }
}

void
ComboBox::mouseReleaseEvent(QMouseEvent* e)
{
    _clicked = false;
    repaint();
    QFrame::mouseReleaseEvent(e);
}

void
ComboBox::wheelEvent(QWheelEvent *e)
{
    if (!hasFocus()) {
        return;
    }
    if (e->delta()>0) {
        setCurrentIndex((activeIndex() - 1 < 0) ? count() - 1 : activeIndex() - 1);
    } else {
        setCurrentIndex((activeIndex() + 1) % count());
    }
}

void
ComboBox::keyPressEvent(QKeyEvent* e)
{
    if (!hasFocus()) {
        return;
    } else {
        if (e->key() == Qt::Key_Up) {
            setCurrentIndex((activeIndex() - 1 < 0) ? count() - 1 : activeIndex() - 1);
        } else if (e->key() == Qt::Key_Down) {
            setCurrentIndex((activeIndex() + 1) % count());
        } else {
            QFrame::keyPressEvent(e);
        }
    }
}

void
ComboBox::createMenu()
{
    _menu->clear();
    for (U32 i = 0; i < _actions.size(); ++i) {
        for (U32 j = 0; j < _separators.size(); ++j) {
            if (_separators[j] == (int)i) {
                _menu->addSeparator();
                break;
            }
        }
        _actions[i]->setEnabled( _enabled && !_readOnly );
        _menu->addAction(_actions[i]);
    }
    QAction* triggered = _menu->exec( this->mapToGlobal( QPoint( 0,height() ) ) );
    for (U32 i = 0; i < _actions.size(); ++i) {
        if (triggered == _actions[i]) {
            setCurrentIndex(i);
            break;
        }
    }
    _clicked = false;
    setFocus();
    repaint();
}

int
ComboBox::count() const
{
    return (int)_actions.size();
}

void
ComboBox::insertItem(int index,
                     const QString & item,
                     QIcon icon,
                     QKeySequence key,
                     const QString & toolTip)
{
    assert(index >= 0);
    QAction* action =  new QAction(this);
    action->setText(item);
    if ( !toolTip.isEmpty() ) {
        action->setToolTip( Qt::convertFromPlainText(toolTip, Qt::WhiteSpaceNormal) );
    }
    if ( !icon.isNull() ) {
        action->setIcon(icon);
    }
    if ( !key.isEmpty() ) {
        action->setShortcut(key);
    }

    growMaximumWidthFromText(item);
    _actions.insert(_actions.begin() + index, action);
    /*if this is the first action we add, make it current*/
    if (_actions.size() == 1) {
        setCurrentText_no_emit( itemText(0) );
    }
}

void
ComboBox::addAction(QAction* action)
{
    QString text = action->text();

    growMaximumWidthFromText(text);
    action->setParent(this);
    _actions.push_back(action);
    
    /*if this is the first action we add, make it current*/
    if (_actions.size() == 1) {
        setCurrentText_no_emit( itemText(0) );
    }
}

void
ComboBox::addItem(const QString & item,
                  QIcon icon,
                  QKeySequence key,
                  const QString & toolTip)
{
    QAction* action =  new QAction(this);

    action->setText(item);
    if ( !icon.isNull() ) {
        action->setIcon(icon);
    }
    if ( !key.isEmpty() ) {
        action->setShortcut(key);
    }
    if ( !toolTip.isEmpty() ) {
        action->setToolTip( Qt::convertFromPlainText(toolTip, Qt::WhiteSpaceNormal) );
    }
    addAction(action);
}

void
ComboBox::setCurrentText_no_emit(const QString & text)
{
    setCurrentText_internal(text);
}

void
ComboBox::setCurrentText(const QString & text)
{
    int index = setCurrentText_internal(text);

    if (index != -1) {
        emit currentIndexChanged(index);
        emit currentIndexChanged( getCurrentIndexText() );
    }
}

int
ComboBox::setCurrentText_internal(const QString & text)
{
    QString str(text);

    growMaximumWidthFromText(str);
    
    _currentText = text;
    QFontMetrics m = fontMetrics();

    // if no action matches this text, set the index to a dirty value
    int index = -1;
    for (U32 i = 0; i < _actions.size(); ++i) {
        if (_actions[i]->text() == text) {
            index = i;
            break;
        }
    }
    if ( (_currentIndex != index) && (index != -1) ) {
        _currentIndex = index;

        return index;
    }
    updateLabel();
    return -1;
}

void
ComboBox::setMaximumWidthFromText(const QString & str)
{
    int w = fontMetrics().width(str);
    setMaximumWidth(w + DROP_DOWN_ICON_SIZE * 2);
}

void
ComboBox::growMaximumWidthFromText(const QString & str)
{
    int w = fontMetrics().width(str) + 2 * DROP_DOWN_ICON_SIZE;

    if ( w > maximumWidth() ) {
        setMaximumWidth(w);
    }
}

int
ComboBox::activeIndex() const
{
    return _currentIndex;
}

QString
ComboBox::getCurrentIndexText() const
{
    assert( _currentIndex < (int)_actions.size() );

    return _actions[_currentIndex]->text();
}

bool
ComboBox::setCurrentIndex_internal(int index)
{
    QString str;
    QString text;

    if ( (0 <= index) && ( index < (int)_actions.size() ) ) {
        text = _actions[index]->text();
    }
    str = text;

    _currentText = str;
    QFontMetrics m = fontMetrics();
    setMinimumWidth( m.width(str) + 2 * DROP_DOWN_ICON_SIZE);

    if (index != -1) {
        _currentIndex = index;
        updateLabel();
        return true;
    } else {
        return false;
    }
}

void
ComboBox::setCurrentIndex(int index)
{
    if ( setCurrentIndex_internal(index) ) {
        ///emit the signal only if the entry changed
        emit currentIndexChanged(_currentIndex);
        emit currentIndexChanged( getCurrentIndexText() );
    }
}

void
ComboBox::setCurrentIndex_no_emit(int index)
{
    setCurrentIndex_internal(index);
}

void
ComboBox::addSeparator()
{
    _separators.push_back(_actions.size() - 1);
}

void
ComboBox::insertSeparator(int index)
{
    assert(index >= 0);
    _separators.push_back(index);
}

QString
ComboBox::itemText(int index) const
{
    if ( (0 <= index) && ( index < (int)_actions.size() ) ) {
        assert(_actions[index]);

        return _actions[index]->text();
    } else {
        return "";
    }
}

int
ComboBox::itemIndex(const QString & str) const
{
    for (U32 i = 0; i < _actions.size(); ++i) {
        if (_actions[i]->text() == str) {
            return i;
        }
    }

    return -1;
}

void
ComboBox::removeItem(const QString & item)
{
    for (U32 i = 0; i < _actions.size(); ++i) {
        assert(_actions[i]);
        if (_actions[i]->text() == item) {
            QString currentText = getCurrentIndexText();
            _actions.erase(_actions.begin() + i);
            if (currentText == item) {
                setCurrentIndex(i - 1);
            }
            /*adjust separators that were placed after this item*/
            for (U32 j = 0; j < _separators.size(); ++j) {
                if (_separators[j] >= (int)i) {
                    --_separators[j];
                }
            }
        }
    }

}

void
ComboBox::clear()
{
    _actions.clear();
    _menu->clear();
    _separators.clear();
    _currentIndex = 0;
    updateLabel();
}

void
ComboBox::setItemText(int index,
                      const QString & item)
{
    assert( 0 <= index && index < (int)_actions.size() );
    assert(_actions[index]);
    _actions[index]->setText(item);
    growMaximumWidthFromText(item);
    if (index == _currentIndex) {
        setCurrentText_internal(item);
    }

}

void
ComboBox::setItemShortcut(int index,
                          const QKeySequence & sequence)
{
    assert( 0 <= index && index < (int)_actions.size() );
    assert(_actions[index]);
    _actions[index]->setShortcut(sequence);
}

void
ComboBox::setItemIcon(int index,
                      const QIcon & icon)
{
    assert( 0 <= index && index < (int)_actions.size() );
    assert(_actions[index]);
    _actions[index]->setIcon(icon);
}

void
ComboBox::disableItem(int index)
{
    assert( 0 <= index && index < (int)_actions.size() );
    assert(_actions[index]);
    _actions[index]->setEnabled(false);
}

void
ComboBox::enableItem(int index)
{
    assert( 0 <= index && index < (int)_actions.size() );
    assert(_actions[index]);
    _actions[index]->setEnabled(true);
}

void
ComboBox::setReadOnly(bool readOnly)
{
    _readOnly = readOnly;
    repaint();
}

void
ComboBox::setEnabled_natron(bool enabled)
{
    _enabled = enabled;
    repaint();
}

int
ComboBox::getAnimation() const
{
    return _animation;
}

void
ComboBox::setAnimation(int i)
{
    _animation = i;
    repaint();
}

void
ComboBox::setDirty(bool b)
{
    _dirty = b;
    repaint();
}


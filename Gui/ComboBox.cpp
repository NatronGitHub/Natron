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

#include "Gui/ComboBox.h"

#include <cassert>
#include <algorithm> // min, max

#include <QLayout>
#include <QStyle>
#include <QFont>
#include <QStyleOption>
#include <QFontMetrics>
#include <QDebug>
#include <QPainter>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated-register)

#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Menu.h"
#include "Gui/ClickableLabel.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Utils.h"


using namespace Natron;


/*
 Copied from QAction.cpp: internal: guesses a descriptive text from a text suited for a menu entry
 */
static QString strippedText(QString s)
{
    s.remove( QString::fromLatin1("...") );
    int i = 0;
    while (i < s.size()) {
        ++i;
        if (s.at(i-1) != QLatin1Char('&'))
            continue;
        if (i < s.size() && s.at(i) == QLatin1Char('&'))
            ++i;
        s.remove(i-1,1);
    }
    return s.trimmed();
}

ComboBox::ComboBox(QWidget* parent)
    : QFrame(parent)
    , _readOnly(false)
    , _enabled(true)
    , _animation(0)
    , _clicked(false)
    , _dirty(false)
    , _altered(false)
    , _cascading(false)
    , _cascadingIndex(0)
    , _currentIndex(0)
    , _currentText()
    , _separators()
    , _rootNode(new ComboBoxMenuNode())
    , _sh()
    , _msh()
    , _sizePolicy()
    , _validHints(false)
    , _align(Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs)
{

    setFrameShape(QFrame::Box);

    setCurrentIndex(0);

    _rootNode->isMenu = new MenuWithToolTips(this);

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
    Q_EMIT minimumSizeChanged(_msh);
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
            double r,g,b;
            switch (_animation) {
                case 0:
                default: {
                    
                    appPTR->getCurrentSettings()->getRaisedColor(&r, &g, &b);
                }   break;
                case 1: {
                    appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
                }   break;
                case 2:
                {
                    appPTR->getCurrentSettings()->getKeyframeColor(&r, &g, &b);
                }   break;
                case 3:
                {
                    appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
                }   break;
            }
            fillColor.setRgb(Color::floatToInt<256>(r),
                             Color::floatToInt<256>(g),
                             Color::floatToInt<256>(b));

        }
        
        double fw = frameWidth();

        QPen pen;
        if (!hasFocus()) {
            pen.setColor(Qt::black);
        } else {
            double r,g,b;
            appPTR->getCurrentSettings()->getSelectionColor(&r, &g, &b);
            QColor c;
            c.setRgb(Color::floatToInt<256>(r),
                     Color::floatToInt<256>(g),
                     Color::floatToInt<256>(b));
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
    } else if (_altered) {
        double aR,aG,aB;
        appPTR->getCurrentSettings()->getAltTextColor(&aR, &aG, &aB);
        textColor.setRgbF(aR, aG, aB);
    } else if (!_enabled) {
        textColor = Qt::black;
    } else {
        double r,g,b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        textColor.setRgb(Color::floatToInt<256>(r),
                         Color::floatToInt<256>(g),
                         Color::floatToInt<256>(b));
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
    if ( buttonDownIsLeft(e) && !_readOnly ) {
        _clicked = true;
        createMenu();
        update();
        QFrame::mousePressEvent(e);
    }
}

void
ComboBox::mouseReleaseEvent(QMouseEvent* e)
{
    _clicked = false;
    update();
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
        int c = count();
        if (c != 0) {
            setCurrentIndex((activeIndex() + 1) % c);
        }
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

static void setEnabledRecursive(bool enabled, ComboBoxMenuNode* node)
{
    if (node->isLeaf) {
        node->isLeaf->setEnabled(enabled);
    } else {
        assert(node->isMenu);
        for (std::size_t i = 0; i < node->children.size(); ++i) {
            setEnabledRecursive(enabled,node->children[i].get());
        }
    }
}

void
ComboBox::createMenu()
{
    if (!_cascading) {
        _rootNode->isMenu->clear();
        for (U32 i = 0; i < _rootNode->children.size(); ++i) {
            _rootNode->children[i]->isLeaf->setEnabled( _enabled && !_readOnly );
            _rootNode->isMenu->addAction(_rootNode->children[i]->isLeaf);
            for (U32 j = 0; j < _separators.size(); ++j) {
                if (_separators[j] == (int)i) {
                    _rootNode->isMenu->addSeparator();
                    break;
                }
            }
        }
    } else {
        setEnabledRecursive(_enabled && !_readOnly,_rootNode.get());
    }
    QAction* triggered = _rootNode->isMenu->exec( this->mapToGlobal( QPoint( 0,height() ) ) );
    if (triggered) {
        QVariant data = triggered->data();
        if (data.toString() != "New") {
            setCurrentIndex(data.toInt());
        } else {
            Q_EMIT itemNewSelected();
        }
        
    }
    _clicked = false;
    setFocus();
    update();
}

int
ComboBox::count() const
{
    return _cascading ? _cascadingIndex :(int)_rootNode->children.size();
}

void
ComboBox::insertItem(int index,
                     const QString & item,
                     QIcon icon,
                     QKeySequence key,
                     const QString & toolTip)
{
    if (_cascading) {
        qDebug() << "Combobox::insertItem is unsupported when in cascading mode.";
        return;
    }
    
    assert(index >= 0);
    QAction* action =  new QAction(this);
    action->setText(item);
    action->setData(QVariant(index));
    if ( !toolTip.isEmpty() ) {
        action->setToolTip( Natron::convertFromPlainText(toolTip.trimmed(), Qt::WhiteSpaceNormal) );
    }
    if ( !icon.isNull() ) {
        action->setIcon(icon);
    }
    if ( !key.isEmpty() ) {
        action->setShortcut(key);
    }

    growMaximumWidthFromText(item);
    boost::shared_ptr<ComboBoxMenuNode> node(new ComboBoxMenuNode());
    node->text = item;
    node->isLeaf = action;
    node->parent = _rootNode.get();
    _rootNode->isMenu->addAction(node->isLeaf);
    _rootNode->children.insert(_rootNode->children.begin() + index, node);
    /*if this is the first action we add, make it current*/
    if (_rootNode->children.size() == 1) {
        setCurrentText_no_emit( itemText(0) );
    }
}

void
ComboBox::addAction(QAction* action)
{
    action->setData(QVariant((int)_rootNode->children.size()));
    addActionPrivate(action);
}

void
ComboBox::addActionPrivate(QAction* action)
{
    QString text = action->text();
    
    growMaximumWidthFromText(text);
    action->setParent(this);
    boost::shared_ptr<ComboBoxMenuNode> node(new ComboBoxMenuNode());
    node->text = text;
    node->isLeaf = action;
    node->parent = _rootNode.get();
    _rootNode->isMenu->addAction(node->isLeaf);
    _rootNode->children.push_back(node);
    
    /*if this is the first action we add, make it current*/
    if (_rootNode->children.size() == 1) {
        setCurrentText_no_emit( itemText(0) );
    }

}

void
ComboBox::addItemNew()
{
    if (_cascading) {
        qDebug() << "ComboBox::addItemNew unsupported when in cascading mode";
        return;
    }
    QAction* action =  new QAction(this);
    action->setText("New");
    action->setData("New");
    QFont f = QFont(appFont,appFontSize);
    f.setItalic(true);
    action->setFont(f);
    addActionPrivate(action);

}

struct LexicalOrder {
    bool operator() (const boost::shared_ptr<ComboBoxMenuNode>& lhs,const boost::shared_ptr<ComboBoxMenuNode>& rhs) {
        return lhs->text < rhs->text;
    }
};


void
ComboBox::addItem(const QString & item,
                  QIcon icon,
                  QKeySequence key,
                  const QString & toolTip)
{
    if (!_cascading) {
        QAction* action =  new QAction(this);
        
        action->setText(item);
        if ( !icon.isNull() ) {
            action->setIcon(icon);
        }
        if ( !key.isEmpty() ) {
            action->setShortcut(key);
        }
        if ( !toolTip.isEmpty() ) {
            action->setToolTip( Natron::convertFromPlainText(toolTip.trimmed(), Qt::WhiteSpaceNormal) );
        }

        addAction(action);
    } else {
        QStringList splits = item.split('/');
        QStringList realSplits;
        for (int i = 0; i < splits.size(); ++i) {
            if (splits[i].isEmpty() || (splits[i].size() == 1 && splits[i] == "/")) {
                continue;
            }
            realSplits.push_back(splits[i]);
        }
        if (realSplits.isEmpty()) {
            qDebug() << "ComboBox::addItem: Invalid item name for cascading mode:" << item;
            return;
        }
        ComboBoxMenuNode* menuToFind = _rootNode.get();
        
        for (int i = 0; i < realSplits.size(); ++i) {
            ComboBoxMenuNode* found = 0;
            for (std::vector<boost::shared_ptr<ComboBoxMenuNode> >::iterator it = menuToFind->children.begin();
                 it!=menuToFind->children.end(); ++it) {
                if ((*it)->text == realSplits[i]) {
                    found = it->get();
                    break;
                }
            }
            if (found) {
                menuToFind = found;
            } else {
                boost::shared_ptr<ComboBoxMenuNode> node(new ComboBoxMenuNode());
                node->text = realSplits[i];
                node->parent = menuToFind;
                menuToFind->children.push_back(node);
                std::sort(menuToFind->children.begin(),menuToFind->children.end(),LexicalOrder());
                if (i == (realSplits.size() - 1)) {
                    QAction* action =  new QAction(this);
                    
                    action->setText(realSplits[i]);
                    action->setData(QVariant(_cascadingIndex));
                    ++_cascadingIndex;
                    
                    if ( !icon.isNull() ) {
                        action->setIcon(icon);
                    }
                    if ( !key.isEmpty() ) {
                        action->setShortcut(key);
                    }
                    if ( !toolTip.isEmpty() ) {
                        action->setToolTip( Natron::convertFromPlainText(toolTip.trimmed(), Qt::WhiteSpaceNormal) );
                    }

                    node->isLeaf = action;
                    menuToFind->isMenu->addAction(node->isLeaf);
                } else {
                    node->isMenu = new MenuWithToolTips(this);
                    node->isMenu->setTitle(realSplits[i]);
                    menuToFind->isMenu->addAction(node->isMenu->menuAction());
                    menuToFind = node.get();
                }
            }
        }

    }
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
        Q_EMIT currentIndexChanged(index);
        Q_EMIT currentIndexChanged( getCurrentIndexText() );
    }
}

int
ComboBox::setCurrentText_internal(const QString & text)
{
    if (_cascading) {
        qDebug() << "ComboBox::setCurrentText_internal unsupported when in cascading mode";
        return -1;
    }
    QString str(text);

    growMaximumWidthFromText(str);
    
    _currentText = strippedText(text);
    QFontMetrics m = fontMetrics();

    // if no action matches this text, set the index to a dirty value
    int index = -1;
    for (U32 i = 0; i < _rootNode->children.size(); ++i) {
        if (_rootNode->children[i]->text == text) {
            index = i;
            break;
        }
    }
    
    if (_sizePolicy.horizontalPolicy() != QSizePolicy::Fixed) {
        int w = m.width(str) + 2 * DROP_DOWN_ICON_SIZE;
        setMinimumWidth(w);
    }
    
    int ret = -1;
    if ( (_currentIndex != index) && (index != -1) ) {
        _currentIndex = index;
        ret = index;
    }
    updateLabel();
    return ret;
}

void
ComboBox::setMaximumWidthFromText(const QString & str)
{
    if (_sizePolicy.horizontalPolicy() == QSizePolicy::Fixed) {
        return;
    }
    int w = fontMetrics().width(str);
    setMaximumWidth(w + DROP_DOWN_ICON_SIZE * 2);
}

void
ComboBox::growMaximumWidthFromText(const QString & str)
{
    if (_sizePolicy.horizontalPolicy() == QSizePolicy::Fixed) {
        return;
    }
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

static ComboBoxMenuNode* getCurrentIndexNode(int index,ComboBoxMenuNode* node) {
    if (node->isLeaf) {
        if (node->isLeaf->data().toInt() == index) {
            return node;
        } else {
            return 0;
        }
    } else {
        for (std::size_t i = 0; i < node->children.size(); ++i) {
            ComboBoxMenuNode* tmp = getCurrentIndexNode(index, node->children[i].get());
            if (tmp) {
                return tmp;
            }
        }
    }
    return 0;
}

static QString getNodeTextRecursive(ComboBoxMenuNode* node,ComboBoxMenuNode* rootNode) {
    QString ret;
    while (node != rootNode) {
        ret.prepend(node->text);
        if (node->parent) {
            if (node->parent != rootNode) {
                ret.prepend('/');
            }
            node = node->parent;
        }
    }
    return ret;
}

QString
ComboBox::getCurrentIndexText() const
{
    ComboBoxMenuNode* node = getCurrentIndexNode(_currentIndex, _rootNode.get());
    if (!node) {
        return QString();
    }
    return getNodeTextRecursive(node,_rootNode.get());
}



bool
ComboBox::setCurrentIndex_internal(int index)
{
    QString str;
    QString text;
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get());
    if (node) {
        text = getNodeTextRecursive(node,_rootNode.get());
    }
    if (!node) {
        return false;
    }
    //Forbid programmatic setting of the "New" choice, only user can select it
    if ((node->isLeaf && node->isLeaf->data().toString() == "New")) {// "New" choice
        return false;
    }
    
    str = strippedText(text);
    
    if (_sizePolicy.horizontalPolicy() != QSizePolicy::Fixed) {
        QFontMetrics m = fontMetrics();
        int w = m.width(str) + 2 * DROP_DOWN_ICON_SIZE;
        setMinimumWidth(w);
    }
    
    bool ret;
    if ((index != -1 && index != _currentIndex) || _currentText != str) {
        _currentIndex = index;
        _currentText = str;
        ret =  true;
    } else {
        ret = false;
    }
    updateLabel();
    return ret;
}

void
ComboBox::setCurrentIndex(int index)
{
    if (_readOnly || !_enabled) {
        return;
    }
    if ( setCurrentIndex_internal(index) ) {
        ///Q_EMIT the signal only if the entry changed
        Q_EMIT currentIndexChanged(_currentIndex);
        Q_EMIT currentIndexChanged( getCurrentIndexText() );
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
    if (_cascading) {
        qDebug() << "ComboBox separators is unsupported when cascading is enabled";
        return;
    }
    _separators.push_back(_rootNode->children.size() - 1);
}

void
ComboBox::insertSeparator(int index)
{
    if (_cascading) {
        qDebug() << "ComboBox separators is unsupported when cascading is enabled";
        return;
    }
    assert(index >= 0);
    _separators.push_back(index);
}

QString
ComboBox::itemText(int index) const
{
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get()) ;
    if (node) {
        return getNodeTextRecursive(node,_rootNode.get());
    }
    return QString();
}


int
ComboBox::itemIndex(const QString & str) const
{
    if (_cascading) {
        qDebug() << "ComboBox::itemIndex unsupported in cascading mode";
        return -1;
    }
    for (U32 i = 0; i < _rootNode->children.size(); ++i) {
        if (_rootNode->children[i]->text == str) {
            return i;
        }
    }

    return -1;
}

void
ComboBox::removeItem(const QString & item)
{
    
    if (_cascading) {
        qDebug() << "ComboBox::removeItem unsupported in cascading mode";
        return;
    }
    

    for (U32 i = 0; i < _rootNode->children.size(); ++i) {
        assert(_rootNode->children[i]);
        if (_rootNode->children[i]->isLeaf->text() == item) {
            
            
            QString currentText = getCurrentIndexText();
            _rootNode->children.erase(_rootNode->children.begin() + i);
            
            ///Decrease index for all other children
            for (std::size_t j = 0; j < _rootNode->children.size(); ++j) {
                assert(_rootNode->children[j]->isLeaf);
                _rootNode->children[j]->isLeaf->setData(QVariant((int)j));
            }
            
            if (currentText == item) {
                setCurrentIndex(i - 1);
            }
            /*adjust separators that were placed after this item*/
            for (U32 j = 0; j < _separators.size(); ++j) {
                if (_separators[j] >= (int)i) {
                    --_separators[j];
                }
            }
            break;
        }
    }
}

void
ComboBox::clear()
{
    _rootNode->children.clear();
    _rootNode->isMenu->clear();
    _cascadingIndex = 0;
    _separators.clear();
    _currentIndex = 0;
    updateLabel();
}

void
ComboBox::setItemText(int index,
                      const QString & item)
{
    if (_cascading) {
        qDebug() << "ComboBox::setItemText unsupported when in cascading mode";
        return;
    }

    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get()) ;
    if (node) {
        node->text = item;
        if (node->isLeaf) {
            node->isLeaf->setText(item);
        } else if (node->isMenu) {
            node->isMenu->setTitle(item);
        }
    }
    growMaximumWidthFromText(item);
    if (index == _currentIndex) {
        setCurrentText_internal(item);
    }

}

void
ComboBox::setItemShortcut(int index,
                          const QKeySequence & sequence)
{
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get()) ;
    if (node && node->isLeaf) {
        node->isLeaf->setShortcut(sequence);
    }
}

void
ComboBox::setItemIcon(int index,
                      const QIcon & icon)
{
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get()) ;
    if (node && node->isLeaf) {
        node->isLeaf->setIcon(icon);
    }
}

void
ComboBox::disableItem(int index)
{
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get()) ;
    if (node) {
        if (node->isLeaf) {
            node->isLeaf->setEnabled(false);
        } else if (node->isMenu) {
            node->isMenu->setEnabled(false);
        }
    }
}

void
ComboBox::enableItem(int index)
{
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _rootNode.get()) ;
    if (node) {
        if (node->isLeaf) {
            node->isLeaf->setEnabled(true);
        } else if (node->isMenu) {
            node->isMenu->setEnabled(true);
        }
    }
}

void
ComboBox::setReadOnly(bool readOnly)
{
    _readOnly = readOnly;
    update();
}

bool
ComboBox::getEnabled_natron() const
{
    return _enabled;
}

void
ComboBox::setEnabled_natron(bool enabled)
{
    _enabled = enabled;
    update();
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
    update();
}

void
ComboBox::setDirty(bool b)
{
    _dirty = b;
    update();
}

void
ComboBox::setAltered(bool b)
{
    _altered = b;
    update();
}

bool
ComboBox::getAltered() const
{
    return _altered;
}

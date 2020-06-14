/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>

#include <QLayout>
#include <QStyle>
#include <QFont>
#include <QStyleOption>
#include <QFontMetrics>
#include <QtCore/QDebug>
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
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ClickableLabel.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Menu.h"


NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

/*
   Copied from QAction.cpp: internal: guesses a descriptive text from a text suited for a menu entry
 */
static QString
strippedText(QString s)
{
    s.remove( QString::fromLatin1("...") );
    int i = 0;
    while ( i < s.size() ) {
        ++i;
        if ( s.at(i - 1) != QLatin1Char('&') ) {
            continue;
        }
        if ( ( i < s.size() ) && ( s.at(i) == QLatin1Char('&') ) ) {
            ++i;
        }
        s.remove(i - 1, 1);
    }

    return s.trimmed();
}


#if 0
static bool isEqualIntVec(const std::vector<int>& lhs, const std::vector<int>& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    // Handle case of a non multi-selection choice
    if (lhs.size() == 1 && rhs.size() == 1) {
        return lhs.front() != rhs.front();
    }
    std::set<int> rhsSorted;
    for (std::size_t i = 0; i < rhs.size(); ++i) {
        rhsSorted.insert(rhs[i]);
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        std::set<int>::iterator found = rhsSorted.find(lhs[i]);
        if (found == rhsSorted.end()) {
            return false;
        }
    }
    return true;
}
#endif


struct ComboBoxMenuNode
{
    MenuWithToolTips* isMenu;
    QAction* isLeaf;
    QString text;
    std::vector<boost::shared_ptr<ComboBoxMenuNode> > children;
    ComboBoxMenuNode* parent;

    ComboBoxMenuNode()
    : isMenu(0), isLeaf(0), text(), children(), parent(0) {}
};

typedef boost::shared_ptr<ComboBoxMenuNode> ComboBoxMenuNodePtr;


static ComboBoxMenuNode*
getCurrentIndexNode(int index,
                    ComboBoxMenuNode* node)
{
    if (node->isLeaf) {
        if (node->isLeaf->data().toInt() == index) {
            return node;
        } else {
            return 0;
        }
    } else {
        for (std::size_t i = 0; i < node->children.size(); ++i) {
            ComboBoxMenuNode* tmp = getCurrentIndexNode( index, node->children[i].get() );
            if (tmp) {
                return tmp;
            }
        }
    }

    return 0;
}

static ComboBoxMenuNode*
getNodeMatchingLabel(const QString& label,
                    ComboBoxMenuNode* node)
{
    if (node->isLeaf) {
        if (node->isLeaf->text() == label) {
            return node;
        } else {
            return 0;
        }
    } else {
        for (std::size_t i = 0; i < node->children.size(); ++i) {
            ComboBoxMenuNode* tmp = getNodeMatchingLabel( label, node->children[i].get() );
            if (tmp) {
                return tmp;
            }
        }
    }

    return 0;
}

static QString
getNodeTextRecursive(ComboBoxMenuNode* node,
                     ComboBoxMenuNode* rootNode)
{
    QString ret;

    while (node != rootNode) {
        ret.prepend(node->text);
        if (node->parent) {
            if (node->parent != rootNode) {
                ret.prepend( QLatin1Char('/') );
            }
            node = node->parent;
        }
    }

    return ret;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT

struct ComboBox::Implementation
{
    ComboBox* _publicInterface;

    bool enabled;

    // True when user clicked the menu
    bool clicked;

    // Is this a cascading menu with submenus
    bool cascading;

    // Used when populating a cascading menu to assign an index on entries
    int cascadingIndex;

    // True if the user can check 0,1 or multiple actions. If false, only 1 single action is
    // always selected.
    bool multiSelectionEnabled;

    // Selected indices
    std::vector<int> selectedIndices;

    // If not empty, this text will be displayed instead of the labels of selected indices
    QString customText;

    // When a single index is selected and it has a pixmap, this is the pixmap to draw
    QPixmap currentPixmap;

    // True if we should draw the shape
    bool drawShape;

    // A separator is added after all the specified indices
    std::vector<int> separators;

    // The menu nodes containing actions and text
    boost::shared_ptr<ComboBoxMenuNode> rootNode;

    // size hint
    mutable QSize sh;

    // Minimum size hint
    mutable QSize msh;

    // Size policy
    mutable QSizePolicy sizePolicy;

    mutable bool validHints;
    unsigned short align;

     // accumulated wheel delta
    int currentDelta;


    Implementation(ComboBox* publicInterface)
    : _publicInterface(publicInterface)
    , enabled(true)
    , clicked(false)
    , cascading(false)
    , cascadingIndex(0)
    , multiSelectionEnabled(false)
    , selectedIndices()
    , customText()
    , currentPixmap()
    , drawShape(false)
    , separators()
    , rootNode()
    , sh()
    , msh()
    , sizePolicy()
    , validHints(false)
    , align(Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs)
    , currentDelta(0)
    {
        rootNode = boost::make_shared<ComboBoxMenuNode>();
        rootNode->isMenu = createMenu();
    }

    void addActionPrivate(QAction* action);

    MenuWithToolTips* createMenu();

    void growMaximumWidthFromText(const QString & str);
    void populateMenu();

    bool setCurrentIndices(const std::vector<int> &index);

    bool setCurrentLabels(const std::vector<QString>& text);

    void refreshCurrentPixmapFromSelection();

    void refreshActionsCheckedState();

    QSize sizeForWidth(int w) const;

    QRectF layoutRect() const;

    void updateLabel();

    QString getCurrentTextFromSelection() const;

    QColor getFillColor() const;

};

ComboBox::ComboBox(QWidget* parent)
    : QFrame(parent)
    , StyledKnobWidgetBase()
    , ignoreWheelEvent(false)
    , _imp(new Implementation(this))
{
    setFrameShape(QFrame::Box);

    setCurrentIndex(0);

    setSizePolicy( QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed, QSizePolicy::Label) );
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight( TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
}

ComboBox::~ComboBox()
{

}

void
ComboBox::setCascading(bool cascading)
{
    assert(!_imp->multiSelectionEnabled);
    _imp->cascading = cascading;
}

bool
ComboBox::isCascading() const
{
    return _imp->cascading;
}

QString
ComboBox::Implementation::getCurrentTextFromSelection() const
{
    QString ret;
    if (!customText.isEmpty()) {
        return customText;
    }
    if (selectedIndices.empty()) {
        ret = QLatin1String("-");
        return ret;
    }
    for (std::size_t i = 0; i < selectedIndices.size(); ++i) {

        ret += _publicInterface->itemText(selectedIndices[i]);
        if (i < selectedIndices.size() - 1) {
            ret += QLatin1String(", ");
        }
    }
    return ret;
}

MenuWithToolTips*
ComboBox::Implementation::createMenu()
{
    double col[3];
    appPTR->getCurrentSettings()->getRaisedColor(&col[0], &col[1], &col[2]);
    QColor c;
    c.setRgbF(Image::clamp(col[0], 0., 1.), Image::clamp(col[1], 0., 1.), Image::clamp(col[2], 0., 1.));
    QString bgColorStr = QString::fromUtf8("rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue());
    MenuWithToolTips* ret = new MenuWithToolTips(_publicInterface);
    ret->setStyleSheet(QString::fromUtf8("QMenu { background-color: %1; }").arg(bgColorStr));
    return ret;
}

void
ComboBox::setDrawShapeEnabled(bool enabled)
{
    _imp->drawShape = enabled;
}

QSize
ComboBox::Implementation::sizeForWidth(int w) const
{
    if (_publicInterface->minimumWidth() > 0) {
        w = std::max( w, _publicInterface->maximumWidth() );
    }
    QSize contentsMargin;
    {
        QMargins margins = _publicInterface->contentsMargins();
        contentsMargin.setWidth( margins.left() + margins.right() );
        contentsMargin.setHeight( margins.bottom() + margins.top() );
    }
    QRect br;

    //Using this constructor of QFontMetrics will respect the DPI of the screen, see http://doc.qt.io/qt-4.8/qfontmetrics.html#QFontMetrics-2
    QFontMetrics fm(_publicInterface->font(), 0);
    Qt::Alignment align = QStyle::visualAlignment( Qt::LeftToRight, QFlag(this->align) );
    int hextra = DROP_DOWN_ICON_SIZE * 2, vextra = 0;

    ///Indent of 1 character
    int indent = fm.width( QLatin1Char('x') );

    if (indent > 0) {
        if ( (align & Qt::AlignLeft) || (align & Qt::AlignRight) ) {
            hextra += indent;
        }
        if ( (align & Qt::AlignTop) || (align & Qt::AlignBottom) ) {
            vextra += indent;
        }
    }
    // Turn off center alignment in order to avoid rounding errors for centering,
    // since centering involves a division by 2. At the end, all we want is the size.
    int flags = align & ~(Qt::AlignVCenter | Qt::AlignHCenter);
    bool tryWidth = (w < 0) && (align & Qt::TextWordWrap);

    if (tryWidth) {
        w = std::min( fm.averageCharWidth() * 80, _publicInterface->maximumSize().width() );
    } else if (w < 0) {
        w = 2000;
    }

    w -= ( hextra + contentsMargin.width() );

    QString curText = getCurrentTextFromSelection();

    br = fm.boundingRect(0, 0, w, 2000, flags, curText);

    if ( tryWidth && ( br.height() < 4 * fm.lineSpacing() ) && (br.width() > w / 2) ) {
        br = fm.boundingRect(0, 0, w / 2, 2000, flags, curText);
    }

    if ( tryWidth && ( br.height() < 2 * fm.lineSpacing() ) && (br.width() > w / 4) ) {
        br = fm.boundingRect(0, 0, w / 4, 2000, flags, curText);
    }

    const QSize contentsSize(br.width() + hextra, br.height() + vextra);

    return (contentsSize + contentsMargin).expandedTo( _publicInterface->minimumSize() );
} // sizeForWidth

QSize
ComboBox::sizeHint() const
{
    if (_imp->validHints) {
        return minimumSizeHint();
    }

    return _imp->sh;
}

QSize
ComboBox::minimumSizeHint() const
{
    if ( _imp->validHints && ( _imp->sizePolicy == sizePolicy() ) ) {
        return _imp->msh;
    }
    ensurePolished();
    _imp->validHints = true;
    _imp->sh = _imp->sizeForWidth(-1);

    _imp->msh.rheight() = _imp->sizeForWidth(QWIDGETSIZE_MAX).height(); // height for one line
    _imp->msh.rwidth() = _imp->sizeForWidth(0).width();

    if ( _imp->sh.height() < _imp->msh.height() ) {
        _imp->msh.rheight() = _imp->sh.height();
    }
    _imp->sizePolicy = sizePolicy();

    return _imp->msh;
}

void
ComboBox::Implementation::updateLabel()
{
    validHints = false;
    QString curText = getCurrentTextFromSelection();
    growMaximumWidthFromText(curText);
    _publicInterface->updateGeometry(); //< force a call to minmumSizeHint
    _publicInterface->update();
    Q_EMIT _publicInterface->minimumSizeChanged(msh);
}

void
ComboBox::changeEvent(QEvent* e)
{
    if ( (e->type() == QEvent::FontChange) || (e->type() == QEvent::ApplicationFontChange) ||
         ( e->type() == QEvent::ContentsRectChange) ) {
        _imp->updateLabel();
    }
    QFrame::changeEvent(e);
}

void
ComboBox::resizeEvent(QResizeEvent* e)
{
    _imp->updateLabel();
    QFrame::resizeEvent(e);
}

QRectF
ComboBox::Implementation::layoutRect() const
{
    QRect cr = _publicInterface->contentsRect();
    Qt::Alignment align = QStyle::visualAlignment( Qt::LeftToRight, QFlag(this->align) );
    int indent = _publicInterface->fontMetrics().width( QLatin1Char('x') ) / 2;

    if (indent > 0) {
        if (align & Qt::AlignLeft) {
            cr.setLeft(cr.left() + indent);
        }

        if (align & Qt::AlignRight) {
            cr.setRight(cr.right() - indent);
        }

        if (align & Qt::AlignTop) {
            cr.setTop(cr.top() + indent);
        }

        if (align & Qt::AlignBottom) {
            cr.setBottom(cr.bottom() - indent);
        }
    }
    cr.adjust(0, 0, -DROP_DOWN_ICON_SIZE * 2, 0);

    return cr;
}

QColor
ComboBox::Implementation::getFillColor() const
{
    QColor fillColor;

    if (!clicked && !_publicInterface->multipleSelection) {
        double r, g, b;
        switch ((AnimationLevelEnum)_publicInterface->animation) {
            case eAnimationLevelNone:
            default: {
                appPTR->getCurrentSettings()->getRaisedColor(&r, &g, &b);
                break;
            }
            case eAnimationLevelInterpolatedValue: {
                appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
                break;
            }
            case eAnimationLevelOnKeyframe: {
                appPTR->getCurrentSettings()->getKeyframeColor(&r, &g, &b);
                break;
            }
            case eAnimationLevelExpression: {
                appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
                break;
            }
        }
        fillColor.setRgb( Color::floatToInt<256>(r),
                         Color::floatToInt<256>(g),
                         Color::floatToInt<256>(b) );
    }
    return fillColor;
}

void
ComboBox::paintEvent(QPaintEvent* /*e*/)
{

    QPainter p(this);
    QRectF bRect = rect();

    {
        // Draw the frame
        QColor fillColor = _imp->getFillColor();

        double fw = 2;
        QPen pen;
        QColor frameColor;
        double frameColorR, frameColorG, frameColorB;
        if ( !hasFocus() ) {
            appPTR->getCurrentSettings()->getSunkenColor(&frameColorR, &frameColorG, &frameColorB);
        } else {
            appPTR->getCurrentSettings()->getSelectionColor(&frameColorR, &frameColorG, &frameColorB);
        }
        frameColor.setRgb( Color::floatToInt<256>(frameColorR),
                           Color::floatToInt<256>(frameColorG),
                           Color::floatToInt<256>(frameColorB) );
        pen.setColor(frameColor);
        p.setPen(pen);

        QRectF roundedRect = bRect.adjusted(fw / 2., fw / 2., -fw, -fw);


        bRect.adjust(fw, fw, -fw, -fw);

        p.fillRect(bRect, fillColor);


        if (_imp->drawShape) {
            double roundPixels = 3;
            QPainterPath path;
            path.addRoundedRect(roundedRect, roundPixels, roundPixels);
            p.drawPath(path);
        }
    }
    QColor textColor;
    if (!_imp->enabled) {
        textColor.setRgb(0, 0, 0);
    } else if (selected) {
        double aR, aG, aB;
        appPTR->getCurrentSettings()->getSelectionColor(&aR, &aG, &aB);
        textColor.setRgbF(aR, aG, aB);
    } else if (!modified) {
        double aR, aG, aB;
        appPTR->getCurrentSettings()->getAltTextColor(&aR, &aG, &aB);
        textColor.setRgbF(aR, aG, aB);
    } else {
        double r, g, b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        textColor.setRgb( Color::floatToInt<256>(r),
                          Color::floatToInt<256>(g),
                          Color::floatToInt<256>(b) );
    }


    if (_imp->currentPixmap.isNull()) {
        Qt::Alignment align = QStyle::visualAlignment( Qt::LeftToRight, QFlag(_imp->align) );
        int flags = align | Qt::TextForceLeftToRight;

        // Draw the text
        QPen pen = p.pen();
        if (_imp->selectedIndices.empty()) {
            QFont f = p.font();
            f.setItalic(true);
            p.setFont(f);
        }
        pen.setColor(textColor);
        p.setPen(pen);

        QRectF lr = _imp->layoutRect().toAlignedRect();
        QString text = _imp->getCurrentTextFromSelection();
        p.drawText(lr.toRect(), flags, text);
    } else {
        double maxPixWidth = width() * 0.6;
        double maxPixHeight = height() * 0.8;
        double maxSize = std::min(maxPixWidth, maxPixHeight);
        if (_imp->currentPixmap.width() >= maxSize || _imp->currentPixmap.height() >= maxSize) {
            _imp->currentPixmap = _imp->currentPixmap.scaled(maxSize, maxSize);
        }
        QRectF lr = _imp->layoutRect().toAlignedRect();
        QRect pixRect = _imp->currentPixmap.rect();
        QPointF center = lr.center();
        center.rx() -= pixRect.width() / 2.;
        center.ry() -= pixRect.height() / 2.;

        QRect targetRect(center.x(), center.y(), pixRect.width(), pixRect.height());

        QStyleOption opt;
        opt.initFrom(this);
        QPixmap pix = _imp->currentPixmap;
        if (!isEnabled()) {
            pix = style()->generatedIconPixmap(QIcon::Disabled, pix, &opt);
        }
        style()->drawItemPixmap(&p, targetRect, Qt::AlignCenter, pix);

    }

    {
        // Draw the dropdown icon
        QPainterPath path;
        QPolygonF poly;
        poly.push_back( QPointF(bRect.right() - DROP_DOWN_ICON_SIZE * 3. / 2., bRect.height() / 2. - DROP_DOWN_ICON_SIZE / 2.) );
        poly.push_back( QPointF(bRect.right() - DROP_DOWN_ICON_SIZE / 2., bRect.height() / 2. - DROP_DOWN_ICON_SIZE / 2.) );
        poly.push_back( QPointF(bRect.right() - DROP_DOWN_ICON_SIZE, bRect.height() / 2. + DROP_DOWN_ICON_SIZE / 2.) );
        path.addPolygon(poly);
        p.fillPath(path, textColor);
    }
} // ComboBox::paintEvent

void
ComboBox::mousePressEvent(QMouseEvent* e)
{
    if (buttonDownIsLeft(e)) {
        _imp->clicked = true;
        _imp->populateMenu();

        QAction* triggered = _imp->rootNode->isMenu->exec( mapToGlobal( QPoint( 0, height() ) ) );
        if (triggered) {
            QVariant data = triggered->data();
            if ( data.toString() != QString::fromUtf8("New") ) {
                setCurrentIndex( data.toInt() );
            } else {
                Q_EMIT itemNewSelected();
            }
        }
        _imp->clicked = false;
        if (focusPolicy() != Qt::NoFocus) {
            setFocus();
        }

        update();
        QFrame::mousePressEvent(e);
    }
    _imp->currentDelta = 0;
}

void
ComboBox::mouseReleaseEvent(QMouseEvent* e)
{
    _imp->clicked = false;
    update();
    QFrame::mouseReleaseEvent(e);
    _imp->currentDelta = 0;
}

void
ComboBox::wheelEvent(QWheelEvent *e)
{
    if (ignoreWheelEvent) {
        return QFrame::wheelEvent(e);
    }
    if ( !hasFocus() ) {
        return;
    }
    // a standard wheel click is 120
    _imp->currentDelta += e->delta();

    if ( (_imp->currentDelta <= -120) || (120 <= _imp->currentDelta) ) {
        int c = count();
        int i = activeIndex();
        int delta = _imp->currentDelta / 120;
        _imp->currentDelta -= delta * 120;
        i = (i - delta) % c;
        if (i < 0) {
            i += c;
        }
        setCurrentIndex(i);
    }
}

void
ComboBox::keyPressEvent(QKeyEvent* e)
{
    if ( !hasFocus() ) {
        return;
    } else {
        if (e->key() == Qt::Key_Up) {
            setCurrentIndex( (activeIndex() - 1 < 0) ? count() - 1 : activeIndex() - 1 );
        } else if (e->key() == Qt::Key_Down) {
            setCurrentIndex( (activeIndex() + 1) % count() );
        } else {
            QFrame::keyPressEvent(e);
        }
    }
    _imp->currentDelta = 0;
}

static void
setEnabledRecursive(bool enabled,
                    ComboBoxMenuNode* node)
{
    if (node->isLeaf) {
        node->isLeaf->setEnabled(enabled);
    } else {
        assert(node->isMenu);
        for (std::size_t i = 0; i < node->children.size(); ++i) {
            setEnabledRecursive( enabled, node->children[i].get() );
        }
    }
}

void
ComboBox::Implementation::populateMenu()
{
    if (cascading) {
        setEnabledRecursive(enabled, rootNode.get() );
    } else {
        rootNode->isMenu->clear();
        QAction* activeAction = 0;
        for (std::size_t i = 0; i < rootNode->children.size(); ++i) {
            rootNode->children[i]->isLeaf->setEnabled(enabled);
            rootNode->isMenu->addAction(rootNode->children[i]->isLeaf);
            if (!multiSelectionEnabled && (int)i == _publicInterface->activeIndex()) {
                activeAction = rootNode->children[i]->isLeaf;
            }
            for (std::size_t j = 0; j < separators.size(); ++j) {
                if (separators[j] == (int)i) {
                    rootNode->isMenu->addSeparator();
                    break;
                }
            }
        }
        if (activeAction) {
            rootNode->isMenu->setActiveAction(activeAction);
        }
    }

}

int
ComboBox::count() const
{
    return _imp->cascading ? _imp->cascadingIndex : (int)_imp->rootNode->children.size();
}

void
ComboBox::insertItem(int index,
                     const QString & item,
                     QIcon icon,
                     QKeySequence key,
                     const QString & toolTip)
{
    if (_imp->cascading) {
        qDebug() << "Combobox::insertItem is unsupported when in cascading mode.";

        return;
    }

    assert(index >= 0);
    QAction* action =  new QAction(this);
    action->setText(item);
    //if (_imp->multiSelectionEnabled) {
        action->setCheckable(true);
    //}
    action->setData( QVariant(index) );
    if ( !toolTip.isEmpty() ) {
        action->setToolTip( NATRON_NAMESPACE::convertFromPlainText(toolTip.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal) );
    }
    if ( !icon.isNull() ) {
        action->setIcon(icon);
    }
    if ( !key.isEmpty() ) {
        action->setShortcut(key);
    }

    _imp->growMaximumWidthFromText(item);
    boost::shared_ptr<ComboBoxMenuNode> node = boost::make_shared<ComboBoxMenuNode>();
    node->text = item;
    node->isLeaf = action;
    node->parent = _imp->rootNode.get();
    _imp->rootNode->isMenu->addAction(node->isLeaf);
    _imp->rootNode->children.insert(_imp->rootNode->children.begin() + index, node);
    /*if this is the first action we add, make it current*/
    if (_imp->rootNode->children.size() == 1) {
        setCurrentIndexFromLabel( itemText(0), false );
    }
}

void
ComboBox::addAction(QAction* action)
{
    if (_imp->cascading) {
        qDebug() << "ComboBox::addAction is not supported in cascaded mode";
        return;
    }
    action->setData( QVariant( (int)_imp->rootNode->children.size() ) );
    _imp->addActionPrivate(action);
}

void
ComboBox::Implementation::addActionPrivate(QAction* action)
{
    QString text = action->text();

    //if (multiSelectionEnabled) {
        action->setCheckable(true);
    //}

    growMaximumWidthFromText(text);
    action->setParent(_publicInterface);
    boost::shared_ptr<ComboBoxMenuNode> node = boost::make_shared<ComboBoxMenuNode>();
    node->text = text;
    node->isLeaf = action;
    node->parent = rootNode.get();
    rootNode->isMenu->addAction(node->isLeaf);
    rootNode->children.push_back(node);

    // if this is the first action we add, make it current
    if (rootNode->children.size() == 1) {
        _publicInterface->setCurrentIndex(0, false );
    }
}

void
ComboBox::addItemNew()
{
    if (_imp->cascading) {
        qDebug() << "ComboBox::addItemNew unsupported when in cascading mode";
        return;
    }
    QAction* action =  new QAction(this);
    action->setText( QString::fromUtf8("New") );
    action->setData( QString::fromUtf8("New") );
    QFont f = QFont(appFont, appFontSize);
    f.setItalic(true);
    action->setFont(f);
    _imp->addActionPrivate(action);
}

struct LexicalOrder
{
    bool operator() (const ComboBoxMenuNodePtr& lhs,
                     const ComboBoxMenuNodePtr& rhs)
    {
        return lhs->text < rhs->text;
    }
};

void
ComboBox::addItem(const QString & item,
                  QIcon icon,
                  QKeySequence key,
                  const QString & toolTip)
{
    if ( item.isEmpty() ) {
        return;
    }

    if (!_imp->cascading) {
        QAction* action =  new QAction(this);

        action->setText(item);
        if ( !icon.isNull() ) {
            action->setIcon(icon);
        }
        if ( !key.isEmpty() ) {
            action->setShortcut(key);
        }
        if ( !toolTip.isEmpty() ) {
            action->setToolTip( NATRON_NAMESPACE::convertFromPlainText(toolTip.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal) );
        }

        addAction(action);
    } else {
        QStringList splits = item.split( QLatin1Char('/') );
        QStringList realSplits;
        for (int i = 0; i < splits.size(); ++i) {
            if ( splits[i].isEmpty() || ( (splits[i].size() == 1) && ( splits[i] == QString::fromUtf8("/") ) ) ) {
                continue;
            }
            realSplits.push_back(splits[i]);
        }
        if ( realSplits.isEmpty() ) {
            qDebug() << "ComboBox::addItem: Invalid item name for cascading mode:" << item;

            return;
        }
        ComboBoxMenuNode* menuToFind = _imp->rootNode.get();

        for (int i = 0; i < realSplits.size(); ++i) {
            ComboBoxMenuNode* found = 0;
            for (std::vector<ComboBoxMenuNodePtr>::iterator it = menuToFind->children.begin();
                 it != menuToFind->children.end(); ++it) {
                if ( (*it)->text == realSplits[i] ) {
                    found = it->get();
                    break;
                }
            }
            if (found) {
                menuToFind = found;
            } else {
                ComboBoxMenuNodePtr node = boost::make_shared<ComboBoxMenuNode>();
                node->text = realSplits[i];
                node->parent = menuToFind;
                menuToFind->children.push_back(node);
                std::sort( menuToFind->children.begin(), menuToFind->children.end(), LexicalOrder() );
                if ( i == (realSplits.size() - 1) ) {
                    QAction* action =  new QAction(this);
                    //if (_imp->multiSelectionEnabled) {
                        action->setCheckable(true);
                    //}
                    action->setText(realSplits[i]);
                    action->setData( QVariant(_imp->cascadingIndex) );
                    ++_imp->cascadingIndex;

                    if ( !icon.isNull() ) {
                        action->setIcon(icon);
                    }
                    if ( !key.isEmpty() ) {
                        action->setShortcut(key);
                    }
                    if ( !toolTip.isEmpty() ) {
                        action->setToolTip( NATRON_NAMESPACE::convertFromPlainText(toolTip.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal) );
                    }

                    node->isLeaf = action;
                    menuToFind->isMenu->addAction(node->isLeaf);
                } else {
                    node->isMenu = _imp->createMenu();
                    node->isMenu->setTitle(realSplits[i]);
                    menuToFind->isMenu->addAction( node->isMenu->menuAction() );
                    menuToFind = node.get();
                }
            }
        }
    }
} // addItem


void
ComboBox::setCurrentIndex(int index, bool emitSignal)
{
    std::vector<int> vec(1);
    vec[0] = index;
    setSelectedIndices(vec, emitSignal);
}

void
ComboBox::setCurrentIndexFromLabel(const QString & text, bool emitSignal)
{
    std::vector<QString> vec(1);
    vec[0] = text;
    setSelectedIndicesFromLabel(vec, emitSignal);
}

void
ComboBox::setSelectedIndices(const std::vector<int>& indices, bool emitSignal)
{
    if (!_imp->enabled) {
        return;
    }
    assert(!_imp->multiSelectionEnabled || indices.size() == 1);
    if ( _imp->setCurrentIndices(indices) ) {
        if (emitSignal) {
            if (_imp->multiSelectionEnabled) {
                Q_EMIT selectionChanged();
            } else {
                Q_EMIT currentIndexChanged(activeIndex());
                Q_EMIT currentIndexChanged(getCurrentIndexText());
            }
        }
    }
}


void
ComboBox::setSelectedIndicesFromLabel(const std::vector<QString>& labels, bool emitSignal)
{
    if (!_imp->enabled) {
        return;
    }
    if ( _imp->setCurrentLabels(labels) ) {
        if (emitSignal) {
            if (_imp->multiSelectionEnabled) {
                Q_EMIT selectionChanged();
            } else {
                Q_EMIT currentIndexChanged(activeIndex());
                Q_EMIT currentIndexChanged(getCurrentIndexText());
            }
        }
    }
}

static void
setCheckedStateInternal(const std::set<int>& selected,
                    ComboBoxMenuNode* node)
{
    if (node->isLeaf) {
        int nodeIndex = node->isLeaf->data().toInt();
        bool found = selected.find(nodeIndex) != selected.end();
        node->isLeaf->setChecked(found);
    } else {
        for (std::size_t i = 0; i < node->children.size(); ++i) {
            setCheckedStateInternal( selected, node->children[i].get() );

        }
    }

}

void
ComboBox::Implementation::refreshActionsCheckedState()
{
    std::set<int> tmpSet;
    for (std::size_t i = 0; i < selectedIndices.size(); ++i) {
        tmpSet.insert(selectedIndices[i]);
    }
    setCheckedStateInternal(tmpSet, rootNode.get());
}

void
ComboBox::Implementation::refreshCurrentPixmapFromSelection()
{
    // If single selection, set current pixmap to display
    currentPixmap = QPixmap();
    if (selectedIndices.size() == 1) {
        ComboBoxMenuNode* node = getCurrentIndexNode( selectedIndices.front(), rootNode.get() );
        if (node && node->isLeaf) {
            currentPixmap = node->isLeaf->icon().pixmap(TO_DPIX(32), TO_DPIY(32));
        }
    }

}

bool
ComboBox::Implementation::setCurrentIndices(const std::vector<int> &indices)
{
#if 0

    std::vector<QString> labels(indices.size());
    std::vector<ComboBoxMenuNode*> nodes(indices.size());
    for (std::size_t i = 0; i < index.size(); ++i) {
        nodes[i] = getCurrentIndexNode( index[i], rootNode.get() );
        if (!nodes[i]) {
            return false;
        }
        labels[i] = getNodeTextRecursive( nodes[i], rootNode.get() );

        // Forbid programmatic setting of the "New" choice, only user can select it
        if ( ( nodes[i]->isLeaf && ( nodes[i]->isLeaf->data().toString() == QString::fromUtf8("New") ) ) ) { // "New" choice
            return false;
        }
        labels[i] = strippedText(labels[i]);
    }

    QFontMetrics m = _publicInterface->fontMetrics();

    int minWidth = INT_MIN;

    QString fullText;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        int w = m.width(labels[i]);
        minWidth = std::max(minWidth, w);
        fullText += labels[i];
        if (i < labels.size() - 1) {
            fullText += QLatin1String(", ");
        }
    }
    if (minWidth != INT_MIN) {
        minWidth += 2 * DROP_DOWN_ICON_SIZE;
    }

    if (minWidth != INT_MIN && _sizePolicy.horizontalPolicy() != QSizePolicy::Fixed) {
        setMinimumWidth(minWidth);
    }
#endif

    selectedIndices = indices;
    customText.clear();
    refreshActionsCheckedState();
    refreshCurrentPixmapFromSelection();
    updateLabel();

    return true;

}

bool
ComboBox::Implementation::setCurrentLabels(const std::vector<QString>& labels)
{

    bool ok = true;
    selectedIndices.clear();
    for (std::size_t i = 0; i < labels.size(); ++i) {
        ComboBoxMenuNode* node = getNodeMatchingLabel(labels[i], rootNode.get());
        if (!node || ! node->isLeaf) {
            ok = false;
            break;
        }
        int index = node->isLeaf->data().toInt();
        selectedIndices.push_back(index);
    }

    customText.clear();
    if (!ok) {
        // Make a custom string
        for (std::size_t i = 0; i < labels.size(); ++i) {
            customText += strippedText(labels[i]);
            if (i < labels.size() - 1) {
                customText += QLatin1String(", ");
            }
        }
    }

#if 0
    QFontMetrics m = fontMetrics();

    if (_sizePolicy.horizontalPolicy() != QSizePolicy::Fixed) {
        int w = m.width(str) + 2 * DROP_DOWN_ICON_SIZE;
        setMinimumWidth(w);
    }
#endif
    refreshActionsCheckedState();
    refreshCurrentPixmapFromSelection();
    updateLabel();

    return true;
} // setCurrentLabels



void
ComboBox::setMaximumWidthFromText(const QString & str)
{
    if (_imp->sizePolicy.horizontalPolicy() == QSizePolicy::Fixed) {
        return;
    }
    int w = fontMetrics().width(str);
    setMaximumWidth(w + DROP_DOWN_ICON_SIZE * 2);
}

void
ComboBox::setMultiSelectionEnabled(bool enabled)
{
    assert(!_imp->cascading);
    _imp->multiSelectionEnabled = enabled;
}

bool
ComboBox::isMultiSelectionEnabled() const
{
    return _imp->multiSelectionEnabled;
}

void
ComboBox::Implementation::growMaximumWidthFromText(const QString & str)
{
    if (sizePolicy.horizontalPolicy() == QSizePolicy::Fixed) {
        return;
    }
    int w = _publicInterface->fontMetrics().width(str) + 2 * DROP_DOWN_ICON_SIZE;

    if ( w > _publicInterface->maximumWidth() ) {
        _publicInterface->setMaximumWidth(w);
    }
}

int
ComboBox::activeIndex() const
{
    assert(!_imp->multiSelectionEnabled);
    if (_imp->selectedIndices.empty()) {
        return -1;
    }
    assert(_imp->selectedIndices.size() == 1);
    return _imp->selectedIndices.front();
}

const std::vector<int>&
ComboBox::getSelectedIndices() const
{
    return _imp->selectedIndices;
}


QString
ComboBox::getCurrentIndexText() const
{
    int index = activeIndex();
    if (index == -1) {
        return _imp->customText;
    }
    ComboBoxMenuNode* node = getCurrentIndexNode(index, _imp->rootNode.get() );
    if (!node) {
        return QString();
    }

    return getNodeTextRecursive( node, _imp->rootNode.get() );
}


void
ComboBox::addSeparator()
{
    if (_imp->cascading) {
        qDebug() << "ComboBox separators is unsupported when cascading is enabled";

        return;
    }
    _imp->separators.push_back(_imp->rootNode->children.size() - 1);
}

void
ComboBox::insertSeparator(int index)
{
    if (_imp->cascading) {
        qDebug() << "ComboBox separators is unsupported when cascading is enabled";

        return;
    }
    assert(index >= 0);
    _imp->separators.push_back(index);
}

QString
ComboBox::itemText(int index) const
{
    ComboBoxMenuNode* node = getCurrentIndexNode( index, _imp->rootNode.get() );

    if (node) {
        return getNodeTextRecursive( node, _imp->rootNode.get() );
    }

    return QString();
}

int
ComboBox::itemIndex(const QString & str) const
{
    if (_imp->cascading) {
        qDebug() << "ComboBox::itemIndex unsupported in cascading mode";

        return -1;
    }
    for (std::size_t i = 0; i < _imp->rootNode->children.size(); ++i) {
        if (_imp->rootNode->children[i]->text == str) {
            return i;
        }
    }

    return -1;
}

void
ComboBox::removeItem(const QString & item)
{
    if (_imp->cascading) {
        qDebug() << "ComboBox::removeItem unsupported in cascading mode";

        return;
    }


    for (std::size_t i = 0; i < _imp->rootNode->children.size(); ++i) {
        assert(_imp->rootNode->children[i]);
        if (_imp->rootNode->children[i]->isLeaf->text() == item) {
            QString currentText = getCurrentIndexText();
            _imp->rootNode->children.erase(_imp->rootNode->children.begin() + i);

            ///Decrease index for all other children
            for (std::size_t j = 0; j < _imp->rootNode->children.size(); ++j) {
                assert(_imp->rootNode->children[j]->isLeaf);
                _imp->rootNode->children[j]->isLeaf->setData( QVariant( (int)j ) );
            }

            if (currentText == item) {
                setCurrentIndex(i - 1);
            }
            /*adjust separators that were placed after this item*/
            for (std::size_t j = 0; j < _imp->separators.size(); ++j) {
                if (_imp->separators[j] >= (int)i) {
                    --_imp->separators[j];
                }
            }
            break;
        }
    }
}

void
ComboBox::clear()
{
    _imp->rootNode->children.clear();
    _imp->rootNode->isMenu->clear();
    _imp->cascadingIndex = 0;
    _imp->separators.clear();
    _imp->customText.clear();
    _imp->selectedIndices.clear();
    _imp->refreshActionsCheckedState();
    _imp->refreshCurrentPixmapFromSelection();
    _imp->updateLabel();
}

void
ComboBox::setItemText(int index,
                      const QString & item)
{
    if (_imp->cascading) {
        qDebug() << "ComboBox::setItemText unsupported when in cascading mode";

        return;
    }

    ComboBoxMenuNode* node = getCurrentIndexNode( index, _imp->rootNode.get() );
    if (node) {
        node->text = item;
        if (node->isLeaf) {
            node->isLeaf->setText(item);
        } else if (node->isMenu) {
            node->isMenu->setTitle(item);
        }
    }
    _imp->updateLabel();
}

void
ComboBox::setItemShortcut(int index,
                          const QKeySequence & sequence)
{
    ComboBoxMenuNode* node = getCurrentIndexNode( index, _imp->rootNode.get() );

    if (node && node->isLeaf) {
        node->isLeaf->setShortcut(sequence);
    }
}

void
ComboBox::setItemIcon(int index,
                      const QIcon & icon)
{
    ComboBoxMenuNode* node = getCurrentIndexNode( index, _imp->rootNode.get() );

    if (node && node->isLeaf) {
        node->isLeaf->setIcon(icon);
    }
}

void
ComboBox::setItemEnabled(int index, bool enabled)
{
    ComboBoxMenuNode* node = getCurrentIndexNode( index, _imp->rootNode.get() );

    if (node) {
        if (node->isLeaf) {
            node->isLeaf->setEnabled(enabled);
        } else if (node->isMenu) {
            node->isMenu->setEnabled(enabled);
        }
    }
}


bool
ComboBox::getEnabled_natron() const
{
    return _imp->enabled;
}

void
ComboBox::setEnabled_natron(bool enabled)
{
    _imp->enabled = enabled;
    refreshStylesheet();
}

void
ComboBox::refreshStylesheet()
{
    update();
}


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ComboBox.cpp"

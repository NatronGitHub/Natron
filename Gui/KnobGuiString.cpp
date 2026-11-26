/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "KnobGuiString.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/QtCompat.h" // Qt::CTRL | Qt::Key_X with Qt5

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>

NATRON_NAMESPACE_ENTER
using std::make_pair;


//=============================STRING_KNOB_GUI===================================

AnimatingTextEdit::AnimatingTextEdit(const KnobGuiPtr& knob,
                                     int dimension,
                                     QWidget* parent)
    : QTextEdit(parent)
    , animation(0)
    , readOnlyNatron(false)
    , _hasChanged(false)
    , dirty(false)
    , _dnd( KnobWidgetDnD::create(knob, dimension, this) )
{
    setTabStopDistance(20);
}

AnimatingTextEdit::~AnimatingTextEdit()
{
}

void
AnimatingTextEdit::setAnimation(int v)
{
    if (v != animation) {
        animation = v;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
AnimatingTextEdit::setReadOnlyNatron(bool ro)
{
    QTextEdit::setReadOnly(ro);
    if (readOnlyNatron != ro) {
        readOnlyNatron = ro;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
AnimatingTextEdit::setReadOnly(bool ro)
{
    // Should never be called since on Mac is set the WA_ShowFocusRect attribute to 1 which
    // makes the application UI redraw all its widgets for any change.
    assert(false);
    QTextEdit::setReadOnly(ro);
}

void
AnimatingTextEdit::setDirty(bool b)
{
    if (dirty != b) {
        dirty = b;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
AnimatingTextEdit::enterEvent(QtCompat::QEnterEvent* e)
{
    _dnd->mouseEnter(e);
    QTextEdit::enterEvent(e);
}

void
AnimatingTextEdit::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    QTextEdit::leaveEvent(e);
}

void
AnimatingTextEdit::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    if ( modCASIsControl(e) && (e->key() == Qt::Key_Return) ) {
        if (_hasChanged) {
            _hasChanged = false;
            Q_EMIT editingFinished();
        }
    }
    _hasChanged = true;
    QTextEdit::keyPressEvent(e);
}

void
AnimatingTextEdit::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    QTextEdit::keyReleaseEvent(e);
}

void
AnimatingTextEdit::paintEvent(QPaintEvent* e)
{
    QPalette p = this->palette();
    QColor c(200, 200, 200, 255);

    p.setColor( QPalette::Highlight, c );
    //p.setColor( QPalette::HighlightedText, c );
    this->setPalette( p );
    QTextEdit::paintEvent(e);
}

void
AnimatingTextEdit::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        QTextEdit::mousePressEvent(e);
    }
}

void
AnimatingTextEdit::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        QTextEdit::mouseMoveEvent(e);
    }
}

void
AnimatingTextEdit::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    QTextEdit::mouseReleaseEvent(e);
}

void
AnimatingTextEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        QTextEdit::dragEnterEvent(e);
    }
}

void
AnimatingTextEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        QTextEdit::dragMoveEvent(e);
    }
}

void
AnimatingTextEdit::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        QTextEdit::dropEvent(e);
    }
}

void
AnimatingTextEdit::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    QTextEdit::focusInEvent(e);
}

void
AnimatingTextEdit::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    if (_hasChanged) {
        _hasChanged = false;
        Q_EMIT editingFinished();
    }
    QTextEdit::focusOutEvent(e);
}

KnobLineEdit::KnobLineEdit(const KnobGuiPtr& knob,
                           int dimension,
                           QWidget* parent)
    : LineEdit(parent)
    , _dnd( KnobWidgetDnD::create(knob, dimension, this) )
{}

KnobLineEdit::~KnobLineEdit()
{
}

void
KnobLineEdit::enterEvent(QtCompat::QEnterEvent* e)
{
    _dnd->mouseEnter(e);
    LineEdit::enterEvent(e);
}

void
KnobLineEdit::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    LineEdit::leaveEvent(e);
}

void
KnobLineEdit::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    LineEdit::keyPressEvent(e);
}

void
KnobLineEdit::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    LineEdit::keyReleaseEvent(e);
}

void
KnobLineEdit::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        LineEdit::mousePressEvent(e);
    }
}

void
KnobLineEdit::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        LineEdit::mouseMoveEvent(e);
    }
}

void
KnobLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    LineEdit::mouseReleaseEvent(e);
}

void
KnobLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        LineEdit::dragEnterEvent(e);
    }
}

void
KnobLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        LineEdit::dragMoveEvent(e);
    }
}

void
KnobLineEdit::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        LineEdit::dropEvent(e);
    }
}

void
KnobLineEdit::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    LineEdit::focusInEvent(e);
}

void
KnobLineEdit::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    LineEdit::focusOutEvent(e);
}

KnobGuiString::KnobGuiString(KnobIPtr knob,
                             KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _label(0)
    , _container(0)
    , _mainLayout(0)
    , _textEdit(0)
    , _richTextOptions(0)
    , _richTextOptionsLayout(0)
    , _fontCombo(0)
    , _setBoldButton(0)
    , _setItalicButton(0)
    , _fontSizeSpinBox(0)
    , _fontColorButton(0)
    , _fontSize(0)
    , _boldActivated(false)
    , _italicActivated(false)
{
    _knob = std::dynamic_pointer_cast<KnobString>(knob);
}

void
KnobGuiString::createWidget(QHBoxLayout* layout)
{
    KnobStringPtr knob = _knob.lock();

    if ( knob->isMultiLine() ) {
        _container = new QWidget( layout->parentWidget() );
        _mainLayout = new QVBoxLayout(_container);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);

        bool useRichText = knob->usesRichText();
        _textEdit = new AnimatingTextEdit(shared_from_this(), 0, _container);
        _textEdit->setAcceptRichText(useRichText);


        _mainLayout->addWidget(_textEdit);

        QObject::connect( _textEdit, SIGNAL(editingFinished()), this, SLOT(onTextChanged()) );
        // layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_textEdit, 0);

        if ( knob->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(true);
        }

        if (useRichText) {
            _richTextOptions = new QWidget(_container);
            _richTextOptionsLayout = new QHBoxLayout(_richTextOptions);
            _richTextOptionsLayout->setContentsMargins(0, 0, 0, 0);
            _richTextOptionsLayout->setSpacing(8);

            _fontCombo = new QFontComboBox(_richTextOptions);
            _fontCombo->setCurrentFont( QApplication::font() );
            _fontCombo->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _richTextOptionsLayout->addWidget(_fontCombo);

            _fontSizeSpinBox = new SpinBox(_richTextOptions);
            _fontSizeSpinBox->setMinimum(1);
            _fontSizeSpinBox->setMaximum(100);
            _fontSizeSpinBox->setValue(6);
            QObject::connect( _fontSizeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onFontSizeChanged(double)) );
            _fontSizeSpinBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font size."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _richTextOptionsLayout->addWidget(_fontSizeSpinBox);

            QPixmap pixBoldChecked, pixBoldUnchecked, pixItalicChecked, pixItalicUnchecked;
            appPTR->getIcon(NATRON_PIXMAP_BOLD_CHECKED, &pixBoldChecked);
            appPTR->getIcon(NATRON_PIXMAP_BOLD_UNCHECKED, &pixBoldUnchecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_CHECKED, &pixItalicChecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_UNCHECKED, &pixItalicUnchecked);
            QIcon boldIcon;
            boldIcon.addPixmap(pixBoldChecked, QIcon::Normal, QIcon::On);
            boldIcon.addPixmap(pixBoldUnchecked, QIcon::Normal, QIcon::Off);
            _setBoldButton = new Button(boldIcon, QString(), _richTextOptions);
            _setBoldButton->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
            _setBoldButton->setCheckable(true);
            _setBoldButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Bold."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _setBoldButton->setMaximumSize(18, 18);
            QObject::connect( _setBoldButton, SIGNAL(clicked(bool)), this, SLOT(boldChanged(bool)) );
            _richTextOptionsLayout->addWidget(_setBoldButton);

            QIcon italicIcon;
            italicIcon.addPixmap(pixItalicChecked, QIcon::Normal, QIcon::On);
            italicIcon.addPixmap(pixItalicUnchecked, QIcon::Normal, QIcon::Off);

            _setItalicButton = new Button(italicIcon, QString(), _richTextOptions);
            _setItalicButton->setCheckable(true);
            _setItalicButton->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
            _setItalicButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Italic."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _setItalicButton->setMaximumSize(18, 18);
            QObject::connect( _setItalicButton, SIGNAL(clicked(bool)), this, SLOT(italicChanged(bool)) );
            _richTextOptionsLayout->addWidget(_setItalicButton);

            QPixmap pixBlack(15, 15);
            pixBlack.fill(Qt::black);
            _fontColorButton = new Button(QIcon(pixBlack), QString(), _richTextOptions);
            _fontColorButton->setCheckable(false);
            _fontColorButton->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
            _fontColorButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font color."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _fontColorButton->setMaximumSize(18, 18);
            QObject::connect( _fontColorButton, SIGNAL(clicked(bool)), this, SLOT(colorFontButtonClicked()) );
            _richTextOptionsLayout->addWidget(_fontColorButton);

            _richTextOptionsLayout->addStretch();

            _mainLayout->addWidget(_richTextOptions);

            restoreTextInfoFromString();

            ///Connect the slot after restoring
            QObject::connect( _fontCombo, SIGNAL(currentFontChanged(QFont)), this, SLOT(onCurrentFontChanged(QFont)) );
        }

        layout->addWidget(_container);
    } else if ( knob->isLabel() ) {
        const std::string& iconFilePath = knob->getIconLabel();
        if ( !iconFilePath.empty() ) {
            _label = new Label( layout->parentWidget() );

            if ( hasToolTip() ) {
                _label->setToolTip( toolTip() );
            }
            layout->addWidget(_label);
        }
    } else {
        _lineEdit = new KnobLineEdit( shared_from_this(), 0, layout->parentWidget() );

        if ( hasToolTip() ) {
            _lineEdit->setToolTip( toolTip() );
        }
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(_lineEdit);
        QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onLineChanged()) );

        if ( knob->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(true);
        }

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_lineEdit, 0);
    }
} // createWidget

KnobGuiString::~KnobGuiString()
{
}

bool
KnobGuiString::isLabelOnSameColumn() const
{
    KnobStringPtr knob = _knob.lock();
    return knob && knob->isLabel();
}

void
KnobGuiString::removeSpecificGui()
{
    if (_lineEdit) {
        _lineEdit->deleteLater();
    }
    if (_container) {
        _container->deleteLater();
    }
}

std::string
KnobGuiString::getDescriptionLabel() const
{
    KnobStringPtr k = _knob.lock();
    bool isLabel = k->isLabel();

    if (isLabel) {
        return k->getValue();
    } else {
        return k->getLabel();
    }
}

void
KnobGuiString::onLineChanged()
{
    if ( !_lineEdit->isEnabled() || _lineEdit->isReadOnly() ) {
        return;
    }
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string oldText = knob->getValue(0);
    std::string newText = _lineEdit->text().toStdString();

    if (oldText != newText) {
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldText, newText) );
    }
}

QString
KnobGuiString::stripWhitespaces(const QString & str)
{
    ///QString::trimmed() doesn't do the job because it doesn't leave the last character
    ///The code is taken from QString::trimmed
    const QChar* s = str.data();

    if ( !s->isSpace() && !s[str.size() - 1].isSpace() ) {
        return str;
    }

    int start = 0;
    int end = str.size() - 2; ///< end before the last character so we don't remove it

    while ( start <= end && s[start].isSpace() ) { // skip white space from start
        ++start;
    }

    if (start <= end) {                          // only white space
        while ( end && s[end].isSpace() ) {           // skip white space from end
            --end;
        }
    }
    int l = end - start + 2;
    if (l <= 0) {
        return QString();
    }

    return QString(s + start, l);
}

void
KnobGuiString::onTextChanged()
{
    QString txt = _textEdit->toPlainText();

    //txt = stripWhitespaces(txt);
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( knob->usesRichText() ) {
        txt = addHtmlTags(txt);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(0), txt.toStdString() ) );
}

QString
KnobGuiString::addHtmlTags(QString text) const
{
    QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);

    text.prepend(fontTag);
    text.append( QString::fromUtf8(kFontEndTag) );

    if (_boldActivated) {
        text.prepend( QString::fromUtf8(kBoldStartTag) );
        text.append( QString::fromUtf8(kBoldEndTag) );
    }
    if (_italicActivated) {
        text.prepend( QString::fromUtf8(kItalicStartTag) );
        text.append( QString::fromUtf8(kItalicEndTag) );
    }

    ///if the knob had custom data, set them
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return text;
    }
    QString knobOldtext = QString::fromUtf8( knob->getValue(0).c_str() );
    QString startCustomTag( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );
    int startCustomData = knobOldtext.indexOf(startCustomTag);
    if (startCustomData != -1) {
        QString customEndTag( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END) );
        int endCustomData = knobOldtext.indexOf(customEndTag, startCustomData);
        assert(endCustomData != -1);
        startCustomData += startCustomTag.size();

        int fontStart = text.indexOf( QString::fromUtf8(kFontSizeTag) );
        assert(fontStart != -1);

        QString endFontTag( QString::fromUtf8("\">") );
        int fontTagEnd = text.indexOf(endFontTag, fontStart);
        assert(fontTagEnd != -1);
        fontTagEnd += endFontTag.size();

        QString customData = knobOldtext.mid(startCustomData, endCustomData - startCustomData);

        text.insert(fontTagEnd, startCustomTag);
        text.insert(fontTagEnd + startCustomTag.size(), customData);
        text.insert(fontTagEnd + startCustomTag.size() + customData.size(), customEndTag);
    }

    return text;
}

// Whether text starts with startTag and ends with the endTag that closes it: "<b>Note: <b>keep</b></b>" but
// not "<b>Note</b>: <b>keep</b>"
static bool
isWrappedIn(const QString& text,
            const QString& startTag,
            const QString& endTag)
{
    if ( !text.startsWith(startTag) || !text.endsWith(endTag) || (text.size() < startTag.size() + endTag.size()) ) {
        return false;
    }
    int depth = 0;
    int pos = 0;
    for (;;) {
        const int start = text.indexOf(startTag, pos);
        const int end = text.indexOf(endTag, pos);
        if (end == -1) {
            return false;
        }
        if ( (start != -1) && (start < end) ) {
            ++depth;
            pos = start + startTag.size();
        } else {
            --depth;
            pos = end + endTag.size();
            if (depth == 0) {
                return pos == text.size();
            }
        }
    }
}

// The position of the start tag (kBoldStartTag or kItalicStartTag) that the editor adds: before the font tag,
// or, in a label without one, at the start with its end tag at the end; -1 if there is none. A <b> or <i> in
// the text itself is not one.
static int
autoAddedTagPosition(const QString& text,
                     const char* startTag,
                     const char* endTag)
{
    const QString toFind = QString::fromUtf8(startTag);
    const int fontTagStart = text.indexOf( QString::fromUtf8(kFontSizeTag) );

    if (fontTagStart == -1) {
        return isWrappedIn( text, toFind, QString::fromUtf8(endTag) ) ? 0 : -1;
    }
    const int pos = text.lastIndexOf(toFind, fontTagStart);
    if (pos == -1) {
        return -1;
    }
    // its end tag is among the editor's end tags after the font end tag, as removeAutoAddedEndTag() expects:
    // not "<b>Title</b><font ...>" or "<b>Title</b><font ...>x</font> <b>y</b>"
    const QString fontEndTag = QString::fromUtf8(kFontEndTag);
    const int fontEnd = text.lastIndexOf(fontEndTag);
    if (fontEnd == -1) {
        return -1;
    }
    QString tail = text.mid( fontEnd + fontEndTag.size() );
    const QString boldEndTag = QString::fromUtf8(kBoldEndTag);
    const QString italicEndTag = QString::fromUtf8(kItalicEndTag);
    bool found = false;
    while ( !tail.isEmpty() ) {
        const QString next = tail.startsWith(boldEndTag) ? boldEndTag : tail.startsWith(italicEndTag) ? italicEndTag : QString();
        if ( next.isEmpty() ) {
            return -1; // text of the label after the font tag
        }
        found = found || ( next == QString::fromUtf8(endTag) );
        tail.remove( 0, next.size() );
    }

    return found ? pos : -1;
}

// Removes the end tag (kBoldEndTag or kItalicEndTag) that the editor adds: after the font end tag, or at the
// end of a label without one. A </b> or </i> in the text itself is not one.
static void
removeAutoAddedEndTag(QString& text,
                      const char* tag)
{
    const QString toFind = QString::fromUtf8(tag);
    const QString fontEndTag = QString::fromUtf8(kFontEndTag);
    const int fontEnd = text.lastIndexOf(fontEndTag);
    int i;

    // the same test as autoAddedTagPosition(): Natron's font tag, not any </font>
    if ( !text.contains( QString::fromUtf8(kFontSizeTag) ) || (fontEnd == -1) ) {
        i = text.endsWith(toFind) ? text.size() - toFind.size() : -1;
    } else {
        i = text.indexOf(toFind, fontEnd + fontEndTag.size());
    }
    if (i != -1) {
        text.remove( i, toFind.size() );
    }
}

// If text is startTag...endTag around the whole text (isWrappedIn()), removes them and returns true
static bool
unwrapTag(QString& text,
          const char* startTag,
          const char* endTag)
{
    const QString start = QString::fromUtf8(startTag);
    const QString end = QString::fromUtf8(endTag);

    if ( !isWrappedIn(text, start, end) ) {
        return false; // e.g. "<b>a</b> and <b>b</b>"
    }
    text = text.mid( start.size(), text.size() - start.size() - end.size() );

    return true;
}

void
KnobGuiString::restoreTextInfoFromString()
{
    KnobStringPtr knob = _knob.lock();
    QString text = QString::fromUtf8( knob->getValue(0).c_str() );

    if ( !text.contains( QString::fromUtf8(kFontSizeTag) ) ) {
        // An empty label, or one without Natron's font tag (e.g. set from Python, #1807): give it the font
        // shown. Bold and italic around the whole label go outside the font tag, where addHtmlTags() puts
        // them and where the other edits and removeAutoAddedHtmlTags() look for them.
        QString content = text;
        bool bold = false;
        bool italic = false;
        for (;;) {
            if ( !italic && unwrapTag(content, kItalicStartTag, kItalicEndTag) ) {
                italic = true;
            } else if ( !bold && unwrapTag(content, kBoldStartTag, kBoldEndTag) ) {
                bold = true;
            } else {
                break;
            }
        }
        _fontSize = _fontSizeSpinBox->value();
        _fontColor = Qt::black;
        _fontFamily = _fontCombo->currentFont().family();
        text = decorateTextWithFontTag(_fontFamily, _fontSize, _fontColor, content);
        if (bold) {
            text = QString::fromUtf8(kBoldStartTag) + text + QString::fromUtf8(kBoldEndTag);
        }
        if (italic) {
            text = QString::fromUtf8(kItalicStartTag) + text + QString::fromUtf8(kItalicEndTag);
        }
        knob->setValue( text.toStdString() );
    } else {
        // The tag may lack the color or the face, e.g. when the label was set from Python (#1807): these
        // keep their default
        QFont font( _fontCombo->currentFont() );
        font.setPointSize( static_cast<int>( _fontSizeSpinBox->value() ) );
        QColor color(Qt::black);
        parseFont(text, &font, &color);
        _fontFamily = font.family();
        _fontCombo->setCurrentFont( QFont(_fontFamily) );

        _fontSize = font.pointSize();

        _fontSizeSpinBox->setValue(_fontSize);

        _fontColor = color;
    }
    syncBoldItalicWithText(text);
    updateFontColorIcon(_fontColor);
} // restoreTextInfoFromString

void
KnobGuiString::syncBoldItalicWithText(const QString& text)
{
    assert(_setBoldButton && _setItalicButton); // created together, for rich text only
    _italicActivated = autoAddedTagPosition(text, kItalicStartTag, kItalicEndTag) != -1;
    _boldActivated = autoAddedTagPosition(text, kBoldStartTag, kBoldEndTag) != -1;
    _setItalicButton->setChecked(_italicActivated);
    _setItalicButton->setDown(_italicActivated);
    _setBoldButton->setChecked(_boldActivated);
    _setBoldButton->setDown(_boldActivated);
}

void
KnobGuiString::parseFont(const QString & label,
                         QFont *f,
                         QColor *color)
{
    // The label may not have the font tag that the GUI writes (makeFontTag()), or only part of it, e.g. when
    // it was set from Python (see the declaration)
    QString toFind = QString::fromUtf8(kFontSizeTag);
    int startFontTag = label.indexOf(toFind);
    if (startFontTag != -1) {
        startFontTag += toFind.size();
        int j = startFontTag;
        QString sizeStr;
        while ( j < label.size() && label.at(j).isDigit() ) {
            sizeStr.push_back( label.at(j) );
            ++j;
        }
        const int size = sizeStr.toInt();
        if (size > 0) {
            f->setPointSize(size);
        }
    } else {
        startFontTag = 0;
    }

    toFind = QString::fromUtf8(kFontFaceTag);
    startFontTag = label.indexOf(toFind, startFontTag);
    if (startFontTag != -1) {
        startFontTag += toFind.size();
        int j = startFontTag;
        QString faceStr;
        while ( j < label.size() && label.at(j) != QLatin1Char('"') ) {
            faceStr.push_back( label.at(j) );
            ++j;
        }
        f->setFamily(faceStr);
    }

    {
        toFind = QString::fromUtf8(kBoldStartTag);
        int foundBold = label.indexOf(toFind);
        if (foundBold != -1) {
            f->setBold(true);
        }
    }

    {
        toFind = QString::fromUtf8(kItalicStartTag);
        int foundItalic = label.indexOf(toFind);
        if (foundItalic != -1) {
            f->setItalic(true);
        }
    }
    {
        toFind = QString::fromUtf8(kFontColorTag);
        int foundColor = label.indexOf(toFind);
        if (foundColor != -1) {
            foundColor += toFind.size();
            QString currentColor;
            int j = foundColor;
            while ( j < label.size() && label.at(j) != QLatin1Char('"') ) {
                currentColor.push_back( label.at(j) );
                ++j;
            }
            *color = QColor(currentColor);
        }
    }
} // KnobGuiString::parseFont

void
KnobGuiString::updateFontColorIcon(const QColor & color)
{
    QPixmap p(18, 18);

    p.fill(color);
    _fontColorButton->setIcon( QIcon(p) );
}

// Sets the attribute (kFontSizeTag, kFontFaceTag or kFontColorTag) of the font tag that starts at fontTagStart, and adds it
// before the end of the tag if the tag doesn't have it, e.g. a label set from Python (#1807): the tag keeps
// starting with kFontSizeTag, which the other edits look for.
static void
setFontTagAttribute(QString& text,
                    int fontTagStart,
                    const char* attribute,
                    const QString& value)
{
    const QString attributeStart = QString::fromUtf8(attribute);
    const int tagEnd = text.indexOf(QLatin1Char('>'), fontTagStart);
    int i = text.indexOf(attributeStart, fontTagStart);

    if ( (i != -1) && ( (tagEnd == -1) || (i < tagEnd) ) ) {
        i += attributeStart.size();
        int valueEnd = text.indexOf(QLatin1Char('"'), i);
        if (valueEnd == -1) {
            valueEnd = text.size();
        }
        text.replace(i, valueEnd - i, value);
    } else {
        text.insert( (tagEnd == -1) ? text.size() : tagEnd, QLatin1Char(' ') + attributeStart + value + QLatin1Char('"') );
    }
}

void
KnobGuiString::onCurrentFontChanged(const QFont & font)
{
    KnobStringPtr knob = _knob.lock();

    assert(_textEdit);
    QString text = QString::fromUtf8( knob->getValue(0).c_str() );
    //find the first font tag
    QString toFind = QString::fromUtf8(kFontSizeTag);
    int i = text.indexOf(toFind);
    _fontFamily = font.family();
    if (i != -1) {
        setFontTagAttribute(text, i, kFontFaceTag, font.family());
    } else {
        // a label without Natron's font tag, e.g. set from Python (#1807)
        text = decorateTextWithFontTag(_fontFamily, _fontSize, _fontColor, text);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(0), text.toStdString() ) );
}

QString
KnobGuiString::makeFontTag(const QString& family,
                           int fontSize,
                           const QColor& color)
{
    return QString::fromUtf8(kFontSizeTag "%1\" " kFontColorTag "%2\" " kFontFaceTag "%3\">")
           .arg(fontSize)
           .arg( color.name() )
           .arg(family);
}

QString
KnobGuiString::decorateTextWithFontTag(const QString& family,
                                       int fontSize,
                                       const QColor& color,
                                       const QString& text)
{
    return makeFontTag(family, fontSize, color) + text + QString::fromUtf8(kFontEndTag);
}

void
KnobGuiString::onFontSizeChanged(double size)
{
    assert(_textEdit);
    KnobStringPtr knob = _knob.lock();;
    QString text = QString::fromUtf8( knob->getValue(0).c_str() );
    //find the first font tag
    QString toFind = QString::fromUtf8(kFontSizeTag);
    int i = text.indexOf(toFind);
    _fontSize = size;
    if (i == -1) {
        // a label without Natron's font tag, e.g. set from Python (#1807)
        text = decorateTextWithFontTag(_fontFamily, _fontSize, _fontColor, text);
    } else {
        setFontTagAttribute( text, i, kFontSizeTag, QString::number(size) );
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(0), text.toStdString() ) );
}

void
KnobGuiString::boldChanged(bool toggled)
{
    assert(_textEdit);
    KnobStringPtr knob = _knob.lock();
    QString text = QString::fromUtf8( knob->getValue(0).c_str() );
    QString toFind = QString::fromUtf8(kBoldStartTag);
    int i = autoAddedTagPosition(text, kBoldStartTag, kBoldEndTag);

    if ( toggled == (i != -1) ) {
        // the label already is (or isn't) bold, e.g. changed from Python: show it instead of editing it
        syncBoldItalicWithText(text);

        return;
    }
    if (!toggled) {
        text.remove( i, toFind.size() );
        removeAutoAddedEndTag(text, kBoldEndTag);
    } else {
        ///insert right prior to the font size, and after the font end tag: at the start and the end of a label
        ///without them, e.g. set from Python (#1807)
        toFind = QString::fromUtf8(kFontSizeTag);
        i = text.indexOf(toFind);
        text.insert( std::max(i, 0), QString::fromUtf8(kBoldStartTag) );
        toFind = QString::fromUtf8(kFontEndTag);
        i = text.lastIndexOf(toFind);
        text.insert( (i == -1) ? text.size() : (i + toFind.size()), QString::fromUtf8(kBoldEndTag) );
    }

    _boldActivated = toggled;
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(0), text.toStdString() ) );
}

void
KnobGuiString::mergeFormat(const QTextCharFormat & fmt)
{
    QTextCursor cursor = _textEdit->textCursor();

    if ( cursor.hasSelection() ) {
        cursor.mergeCharFormat(fmt);
        _textEdit->mergeCurrentCharFormat(fmt);
    }
}

void
KnobGuiString::colorFontButtonClicked()
{
    assert(_textEdit);
    KnobStringPtr knob = _knob.lock();
    QColorDialog dialog(_textEdit);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(updateFontColorIcon(QColor)) );
    dialog.setCurrentColor(_fontColor);
    if ( dialog.exec() ) {
        _fontColor = dialog.currentColor();

        QString text = QString::fromUtf8( knob->getValue(0).c_str() );
        if ( text.indexOf( QString::fromUtf8(kFontSizeTag) ) == -1 ) {
            // a label without Natron's font tag, e.g. set from Python (#1807)
            text = decorateTextWithFontTag(_fontFamily, _fontSize, _fontColor, text);
        } else {
            findReplaceColorName( text, _fontColor.name() );
        }
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(0), text.toStdString() ) );
    }
    updateFontColorIcon(_fontColor);
}

void
KnobGuiString::findReplaceColorName(QString& text,
                                    const QColor& color)
{
    //find the first font tag
    int i = text.indexOf( QString::fromUtf8(kFontSizeTag) );

    if (i != -1) {
        setFontTagAttribute(text, i, kFontColorTag, color.name());
    }
}

void
KnobGuiString::italicChanged(bool toggled)
{
    KnobStringPtr knob = _knob.lock();
    QString text = QString::fromUtf8( knob->getValue(0).c_str() );
    //find the first font tag
    QString toFind = QString::fromUtf8(kFontSizeTag);
    int i = text.indexOf(toFind);

    if (i == -1) {
        i = 0; // a label without Natron's font tag, e.g. set from Python (#1807): the tags go at the start
    }

    toFind = QString::fromUtf8(kItalicStartTag);
    int foundItalic = autoAddedTagPosition(text, kItalicStartTag, kItalicEndTag);
    if ( toggled == (foundItalic != -1) ) {
        // the label already is (or isn't) italic, e.g. changed from Python: show it instead of editing it
        syncBoldItalicWithText(text);

        return;
    }
    if (!toggled) {
        text.remove( foundItalic, toFind.size() );
        removeAutoAddedEndTag(text, kItalicEndTag);
    } else {
        int foundBold = autoAddedTagPosition(text, kBoldStartTag, kBoldEndTag);

        // right before the editor's bold, or else before the font tag (the start of a label without one)
        text.insert( (foundBold != -1) ? foundBold : i, QString::fromUtf8(kItalicStartTag) );
        text.append( QString::fromUtf8(kItalicEndTag) ); //< this is always the last tag
    }
    _italicActivated = toggled;
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(0), text.toStdString() ) );
}

QString
KnobGuiString::removeNatronHtmlTag(QString text)
{
    ///we also remove any custom data added by natron so the user doesn't see it
    int startCustomData = text.indexOf( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );

    if (startCustomData != -1) {
        QString endTag( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END) );
        int endCustomData = text.indexOf(endTag, startCustomData);
        assert(endCustomData != -1);
        endCustomData += endTag.size();
        text.remove(startCustomData, endCustomData - startCustomData);
    }

    return text;
}

QString
KnobGuiString::getNatronHtmlTagContent(QString text)
{
    QString label = removeAutoAddedHtmlTags(text, false);
    QString startTag = QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START);
    int startCustomData = label.indexOf(startTag);

    if (startCustomData != -1) {
        QString endTag = QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = label.indexOf(endTag, startCustomData);
        assert(endCustomData != -1);
        label = label.remove( endCustomData, endTag.size() );
        label = label.remove( startCustomData, startTag.size() );
    }

    return label;
}

QString
KnobGuiString::removeAutoAddedHtmlTags(QString text,
                                       bool removeNatronTag)
{
    QString toFind = QString::fromUtf8(kFontSizeTag);
    int i = text.indexOf(toFind);
    bool foundFontStart = i != -1;
    // only the tags that the editor adds, as the Bold and Italic buttons see them: a <b> or <i> in the text
    // itself stays (e.g. a label set from Python, "Note: <b>keep</b>")
    // (both decided on the label itself, as syncBoldItalicWithText() does: "<b><i>x</i></b>" keeps its <i>)
    const bool italic = autoAddedTagPosition(text, kItalicStartTag, kItalicEndTag) != -1;
    int foundBold = autoAddedTagPosition(text, kBoldStartTag, kBoldEndTag);
    if (foundBold != -1) {
        text.remove( foundBold, QString::fromUtf8(kBoldStartTag).size() );
        removeAutoAddedEndTag(text, kBoldEndTag);
    }

    int foundItal = italic ? autoAddedTagPosition(text, kItalicStartTag, kItalicEndTag) : -1;
    if (foundItal != -1) {
        text.remove( foundItal, QString::fromUtf8(kItalicStartTag).size() );
        removeAutoAddedEndTag(text, kItalicEndTag);
    }

    ///refresh the index
    i = text.indexOf(toFind);

    QString endTag = QString::fromUtf8("\">");
    int foundEndTag = text.indexOf(endTag, i);
    foundEndTag += endTag.size();
    if (foundFontStart) {
        ///remove the whole font tag
        text.remove(i, foundEndTag - i);
    }

    if (foundFontStart) {
        // a label without Natron's font tag keeps its own </font>
        endTag = QString::fromUtf8(kFontEndTag);
        foundEndTag = text.lastIndexOf(endTag);
        if (foundEndTag != -1) {
            text.remove( foundEndTag, endTag.size() );
        }
    }

    ///we also remove any custom data added by natron so the user doesn't see it
    if (removeNatronTag) {
        return removeNatronHtmlTag(text);
    } else {
        return text;
    }
} // removeAutoAddedHtmlTags

void
KnobGuiString::updateGUI(int /*dimension*/)
{
    KnobStringPtr knob = _knob.lock();
    std::string value = knob->getValue(0);

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->blockSignals(true);
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();
        QString txt = QString::fromUtf8( value.c_str() );
        if ( knob->usesRichText() ) {
            // an undo or a setValue() from Python may have added or removed the bold or italic tags
            if (_setBoldButton) {
                syncBoldItalicWithText(txt);
            }
            txt = removeAutoAddedHtmlTags(txt);
        }

        if ( knob->isCustomHTMLText() ) {
            _textEdit->setHtml(txt);
        } else {
            _textEdit->setPlainText(txt);
        }

        if ( pos < txt.size() ) {
            cursor.setPosition(pos);
        } else {
            cursor.movePosition(QTextCursor::End);
        }

        ///restore selection
        cursor.setPosition(selectionStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);

        _textEdit->setTextCursor(cursor);
        _textEdit->blockSignals(false);
    } else if ( knob->isLabel() ) {
        onLabelChanged();

        // If the knob has a label as an icon, set the content of the knob into a label
        const std::string& iconFilePath = knob->getIconLabel();
        if ( _label && !iconFilePath.empty() ) {
            QString txt = QString::fromUtf8( knob->getValue().c_str() );
            txt.replace( QLatin1String("\n"), QLatin1String("<br>") );
            _label->setText(txt);
        }
    } else {
        assert(_lineEdit);
        _lineEdit->setText( QString::fromUtf8( value.c_str() ) );
    }
} // KnobGuiString::updateGUI

void
KnobGuiString::_hide()
{
    KnobStringPtr knob = _knob.lock();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->hide();
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->hide();
        }
    } else {
        assert(_lineEdit);
        _lineEdit->hide();
    }
}

void
KnobGuiString::_show()
{
    KnobStringPtr knob = _knob.lock();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->show();
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->show();
        }
    } else {
        assert(_lineEdit);
        _lineEdit->show();
    }
}

void
KnobGuiString::setEnabled()
{
    KnobStringPtr knob = _knob.lock();
    bool b = knob->isEnabled(0)  &&  knob->getExpression(0).empty();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        //_textEdit->setEnabled(b);
        //_textEdit->setReadOnly(!b);
        if ( !knob->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(!b);
        }
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->setEnabled(b);
        }
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        if ( !knob->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(!b);
        }
    }
}

void
KnobGuiString::reflectAnimationLevel(int /*dimension*/,
                                     AnimationLevelEnum level)
{
    KnobStringPtr knob = _knob.lock();
    int value;

    switch (level) {
    case eAnimationLevelNone:
        value = 0;
        break;
    case eAnimationLevelInterpolatedValue:
        value = 1;
        break;
    case eAnimationLevelOnKeyframe:
        value = 2;

        break;
    default:
        value = 0;
        break;
    }

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->setAnimation(value);
    } else if ( knob->isLabel() ) {
        //assert(_label);
    } else {
        assert(_lineEdit);
        _lineEdit->setAnimation(value);
    }
}

void
KnobGuiString::setReadOnly(bool readOnly,
                           int /*dimension*/)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if (_textEdit) {
        if ( !knob->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(readOnly);
        }
    } else if (_lineEdit) {
        if ( !knob->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(readOnly);
        }
    }
}

void
KnobGuiString::setDirty(bool dirty)
{
    if (_textEdit) {
        _textEdit->setDirty(dirty);
    } else if (_lineEdit) {
        _lineEdit->setDirty(dirty);
    }
}

KnobIPtr
KnobGuiString::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiString::reflectExpressionState(int /*dimension*/,
                                      bool hasExpr)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool isEnabled = knob->isEnabled(0);

    if (_textEdit) {
        _textEdit->setAnimation(3);
        if ( !knob->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(hasExpr || !isEnabled);
        }
    } else if (_lineEdit) {
        _lineEdit->setAnimation(3);
        _lineEdit->setReadOnly_NoFocusRect(hasExpr || !isEnabled);
    }
}

void
KnobGuiString::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        if (_textEdit) {
            KnobStringPtr knob = _knob.lock();
            if (!knob) {
                return;
            }
            bool useRichText = knob->usesRichText();
            if (useRichText) {
                tt += tr("This text area supports html encoding. "
                         "Please check <a href=http://qt-project.org/doc/qt-5/richtext-html-subset.html>Qt website</a> for more info.");
            }
            QKeySequence seq(Qt::CTRL | Qt::Key_Return);
            tt += tr("Use %1 to validate changes made to the text.").arg( seq.toString(QKeySequence::NativeText) );
            _textEdit->setToolTip(tt);
        } else if (_lineEdit) {
            _lineEdit->setToolTip(tt);
        } else if (_label) {
            _label->setToolTip(tt);
        }
    }
}

void
KnobGuiString::reflectModificationsState()
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool hasModif = knob->hasModifications();

    if (_lineEdit) {
        _lineEdit->setAltered(!hasModif);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiString.cpp"

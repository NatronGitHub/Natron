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
#include <QDialogButtonBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

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
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"

#define kFontSizeTag "<font size=\""
#define kFontColorTag "color=\""
#define kFontFaceTag "face=\""
#define kFontEndTag "</font>"
#define kBoldStartTag "<b>"
#define kBoldEndTag "</b>"
#define kItalicStartTag "<i>"
#define kItalicEndTag "</i>"

NATRON_NAMESPACE_ENTER;
using std::make_pair;


//=============================STRING_KNOB_GUI===================================

void
AnimatingTextEdit::setAnimation(int v)
{
    animation = v;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
AnimatingTextEdit::setReadOnlyNatron(bool ro)
{
    setReadOnly(ro);
    readOnlyNatron = ro;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
AnimatingTextEdit::setDirty(bool b)
{
    dirty = b;
    style()->unpolish(this);
    style()->polish(this);
    update();
}


void
AnimatingTextEdit::enterEvent(QEvent* e)
{
    mouseEnter(e);
    QTextEdit::enterEvent(e);
}

void
AnimatingTextEdit::leaveEvent(QEvent* e)
{
    mouseLeave(e);
    QTextEdit::leaveEvent(e);
}

void
AnimatingTextEdit::keyPressEvent(QKeyEvent* e)
{
    keyPress(e);
    if (modCASIsControl(e) && e->key() == Qt::Key_Return) {
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
    keyRelease(e);
    QTextEdit::keyReleaseEvent(e);
}

void
AnimatingTextEdit::paintEvent(QPaintEvent* e)
{
    QPalette p = this->palette();
    QColor c(200,200,200,255);

    p.setColor( QPalette::Highlight, c );
    //p.setColor( QPalette::HighlightedText, c );
    this->setPalette( p );
    QTextEdit::paintEvent(e);
}

void
AnimatingTextEdit::mousePressEvent(QMouseEvent* e)
{
    if (!mousePress(e)) {
        QTextEdit::mousePressEvent(e);
    }
}

void
AnimatingTextEdit::mouseMoveEvent(QMouseEvent* e)
{
    if (!mouseMove(e)) {
        QTextEdit::mouseMoveEvent(e);
    }
}

void
AnimatingTextEdit::mouseReleaseEvent(QMouseEvent* e)
{
    mouseRelease(e);
    QTextEdit::mouseReleaseEvent(e);
    
}

void
AnimatingTextEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if (!dragEnter(e)) {
        AnimatingTextEdit::dragEnterEvent(e);
    }
}

void
AnimatingTextEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if (!dragMove(e)) {
        AnimatingTextEdit::dragMoveEvent(e);
    }
}
void
AnimatingTextEdit::dropEvent(QDropEvent* e)
{
    if (!drop(e)) {
        AnimatingTextEdit::dropEvent(e);
    }
}

void
AnimatingTextEdit::focusInEvent(QFocusEvent* e)
{
    focusIn();
    QTextEdit::focusInEvent(e);
}

void
AnimatingTextEdit::focusOutEvent(QFocusEvent* e)
{
    focusOut();
    if (_hasChanged) {
        _hasChanged = false;
        Q_EMIT editingFinished();
    }
    QTextEdit::focusOutEvent(e);
    
}


void
KnobLineEdit::enterEvent(QEvent* e)
{
    mouseEnter(e);
    LineEdit::enterEvent(e);
}

void
KnobLineEdit::leaveEvent(QEvent* e)
{
    mouseLeave(e);
    LineEdit::leaveEvent(e);
}

void
KnobLineEdit::keyPressEvent(QKeyEvent* e)
{
    keyPress(e);
    LineEdit::keyPressEvent(e);
}

void
KnobLineEdit::keyReleaseEvent(QKeyEvent* e)
{
    keyRelease(e);
    LineEdit::keyReleaseEvent(e);
}

void
KnobLineEdit::mousePressEvent(QMouseEvent* e)
{
    if (!mousePress(e)) {
        LineEdit::mousePressEvent(e);
    }
}

void
KnobLineEdit::mouseMoveEvent(QMouseEvent* e)
{
    if (!mouseMove(e)) {
        LineEdit::mouseMoveEvent(e);
    }
}

void
KnobLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    mouseRelease(e);
    LineEdit::mouseReleaseEvent(e);
    
}

void
KnobLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if (!dragEnter(e)) {
        LineEdit::dragEnterEvent(e);
    }
}

void
KnobLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if (!dragMove(e)) {
        LineEdit::dragMoveEvent(e);
    }
}
void
KnobLineEdit::dropEvent(QDropEvent* e)
{
    if (!drop(e)) {
        LineEdit::dropEvent(e);
    }
}

void
KnobLineEdit::focusInEvent(QFocusEvent* e)
{
    focusIn();
    LineEdit::focusInEvent(e);
}

void
KnobLineEdit::focusOutEvent(QFocusEvent* e)
{
    focusOut();
    LineEdit::focusOutEvent(e);
}

KnobGuiString::KnobGuiString(KnobPtr knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
      , _lineEdit(0)
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
    _knob = boost::dynamic_pointer_cast<KnobString>(knob);
}

void
KnobGuiString::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
    if ( knob->isMultiLine() ) {
        _container = new QWidget( layout->parentWidget() );
        _mainLayout = new QVBoxLayout(_container);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);

        bool useRichText = knob->usesRichText();
        _textEdit = new AnimatingTextEdit(this, 0, _container);
        _textEdit->setAcceptRichText(useRichText);


        _mainLayout->addWidget(_textEdit);

        QObject::connect( _textEdit, SIGNAL( editingFinished() ), this, SLOT( onTextChanged() ) );
       // layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_textEdit,0);
        
        if (knob->isCustomHTMLText()) {
            _textEdit->setReadOnlyNatron(true);
        }

        if (useRichText) {
            _richTextOptions = new QWidget(_container);
            _richTextOptionsLayout = new QHBoxLayout(_richTextOptions);
            _richTextOptionsLayout->setContentsMargins(0, 0, 0, 0);
            _richTextOptionsLayout->setSpacing(8);

            _fontCombo = new QFontComboBox(_richTextOptions);
            _fontCombo->setCurrentFont(QApplication::font());
            _fontCombo->setToolTip(GuiUtils::convertFromPlainText(tr("Font."), Qt::WhiteSpaceNormal));
            _richTextOptionsLayout->addWidget(_fontCombo);

            _fontSizeSpinBox = new SpinBox(_richTextOptions);
            _fontSizeSpinBox->setMinimum(1);
            _fontSizeSpinBox->setMaximum(100);
            _fontSizeSpinBox->setValue(6);
            QObject::connect( _fontSizeSpinBox,SIGNAL( valueChanged(double) ),this,SLOT( onFontSizeChanged(double) ) );
            _fontSizeSpinBox->setToolTip(GuiUtils::convertFromPlainText(tr("Font size."), Qt::WhiteSpaceNormal));
            _richTextOptionsLayout->addWidget(_fontSizeSpinBox);

            QPixmap pixBoldChecked,pixBoldUnchecked,pixItalicChecked,pixItalicUnchecked;
            appPTR->getIcon(NATRON_PIXMAP_BOLD_CHECKED,&pixBoldChecked);
            appPTR->getIcon(NATRON_PIXMAP_BOLD_UNCHECKED,&pixBoldUnchecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_CHECKED,&pixItalicChecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_UNCHECKED,&pixItalicUnchecked);
            QIcon boldIcon;
            boldIcon.addPixmap(pixBoldChecked,QIcon::Normal,QIcon::On);
            boldIcon.addPixmap(pixBoldUnchecked,QIcon::Normal,QIcon::Off);
            _setBoldButton = new Button(boldIcon,"",_richTextOptions);
            _setBoldButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE,NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _setBoldButton->setCheckable(true);
            _setBoldButton->setToolTip(GuiUtils::convertFromPlainText(tr("Bold."), Qt::WhiteSpaceNormal));
            _setBoldButton->setMaximumSize(18, 18);
            QObject::connect( _setBoldButton,SIGNAL( clicked(bool) ),this,SLOT( boldChanged(bool) ) );
            _richTextOptionsLayout->addWidget(_setBoldButton);

            QIcon italicIcon;
            italicIcon.addPixmap(pixItalicChecked,QIcon::Normal,QIcon::On);
            italicIcon.addPixmap(pixItalicUnchecked,QIcon::Normal,QIcon::Off);

            _setItalicButton = new Button(italicIcon,"",_richTextOptions);
            _setItalicButton->setCheckable(true);
            _setItalicButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE,NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _setItalicButton->setToolTip(GuiUtils::convertFromPlainText(tr("Italic."), Qt::WhiteSpaceNormal));
            _setItalicButton->setMaximumSize(18,18);
            QObject::connect( _setItalicButton,SIGNAL( clicked(bool) ),this,SLOT( italicChanged(bool) ) );
            _richTextOptionsLayout->addWidget(_setItalicButton);

            QPixmap pixBlack(15,15);
            pixBlack.fill(Qt::black);
            _fontColorButton = new Button(QIcon(pixBlack),"",_richTextOptions);
            _fontColorButton->setCheckable(false);
            _fontColorButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE,NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _fontColorButton->setToolTip(GuiUtils::convertFromPlainText(tr("Font color."), Qt::WhiteSpaceNormal));
            _fontColorButton->setMaximumSize(18, 18);
            QObject::connect( _fontColorButton, SIGNAL( clicked(bool) ), this, SLOT( colorFontButtonClicked() ) );
            _richTextOptionsLayout->addWidget(_fontColorButton);

            _richTextOptionsLayout->addStretch();

            _mainLayout->addWidget(_richTextOptions);

            restoreTextInfoFromString();

            ///Connect the slot after restoring
            QObject::connect( _fontCombo,SIGNAL( currentFontChanged(QFont) ),this,SLOT( onCurrentFontChanged(QFont) ) );
        }

        layout->addWidget(_container);
    } else if ( knob->isLabel() ) {
        /*_label = new Label( layout->parentWidget() );

        if ( hasToolTip() ) {
            _label->setToolTip( toolTip() );
        }
        //_label->setFont(QFont(appFont,appFontSize));
        layout->addWidget(_label);*/
    } else {
        _lineEdit = new KnobLineEdit(this, 0, layout->parentWidget() );

        if ( hasToolTip() ) {
            _lineEdit->setToolTip( toolTip() );
        }
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(_lineEdit);
        QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onLineChanged() ) );

        if ( knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(true);
        }

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_lineEdit,0);
    }
} // createWidget

KnobGuiString::~KnobGuiString()
{
}

void KnobGuiString::removeSpecificGui()
{
    delete _lineEdit;
    delete _container;
}

void
KnobGuiString::onLineChanged()
{
    if (!_lineEdit->isEnabled() || _lineEdit->isReadOnly()) {
        return;
    }
    std::string oldText = _knob.lock()->getValue(0);
    std::string newText = _lineEdit->text().toStdString();
   
    if (oldText != newText) {
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldText,newText) );
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

    txt = stripWhitespaces(txt);
    if ( _knob.lock()->usesRichText() ) {
        txt = addHtmlTags(txt);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob.lock()->getValue(0),txt.toStdString() ) );
}

QString
KnobGuiString::addHtmlTags(QString text) const
{
    QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);

    text.prepend(fontTag);
    text.append(kFontEndTag);

    if (_boldActivated) {
        text.prepend(kBoldStartTag);
        text.append(kBoldEndTag);
    }
    if (_italicActivated) {
        text.prepend(kItalicStartTag);
        text.append(kItalicEndTag);
    }

    ///if the knob had custom data, set them
    QString knobOldtext( _knob.lock()->getValue(0).c_str() );
    QString startCustomTag(NATRON_CUSTOM_HTML_TAG_START);
    int startCustomData = knobOldtext.indexOf(startCustomTag);
    if (startCustomData != -1) {
        QString customEndTag(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = knobOldtext.indexOf(customEndTag,startCustomData);
        assert(endCustomData != -1);
        startCustomData += startCustomTag.size();

        int fontStart = text.indexOf(kFontSizeTag);
        assert(fontStart != -1);

        QString endFontTag("\">");
        int fontTagEnd = text.indexOf(endFontTag,fontStart);
        assert(fontTagEnd != -1);
        fontTagEnd += endFontTag.size();

        QString customData = knobOldtext.mid(startCustomData,endCustomData - startCustomData);

        text.insert(fontTagEnd, startCustomTag);
        text.insert(fontTagEnd + startCustomTag.size(), customData);
        text.insert(fontTagEnd + startCustomTag.size() + customData.size(), customEndTag);
    }

    return text;
}

void
KnobGuiString::restoreTextInfoFromString()
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
    QString text( knob->getValue(0).c_str() );

    if ( text.isEmpty() ) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>( knob->getHolder() );
        /// If the node has a sublabel, restore it in the label
        if ( effect && (knob->getName() == kUserLabelKnobName) ) {
            KnobPtr knob = effect->getKnobByName(kNatronOfxParamStringSublabelName);
            if (knob) {
                KnobString* strKnob = dynamic_cast<KnobString*>( knob.get() );
                if (strKnob) {
                    QString sublabel = strKnob->getValue(0).c_str();
                    if (!sublabel.isEmpty()) {
                        text.append(NATRON_CUSTOM_HTML_TAG_START);
                        text.append('(' + sublabel + ')');
                        text.append(NATRON_CUSTOM_HTML_TAG_END);
                    }
                }
            }
        }


        _fontSize = _fontSizeSpinBox->value();
        _fontColor = Qt::black;
        _fontFamily = _fontCombo->currentFont().family();
        _boldActivated = false;
        _italicActivated = false;
        QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);
        text.prepend(fontTag);
        text.append(kFontEndTag);


        knob->setValue(text.toStdString());
    } else {
        QString toFind = QString(kItalicStartTag);
        int i = text.indexOf(toFind);
        if (i != -1) {
            _italicActivated = true;
        } else {
            _italicActivated = false;
        }

        _setItalicButton->setChecked(_italicActivated);
        _setItalicButton->setDown(_italicActivated);

        toFind = QString(kBoldStartTag);
        i = text.indexOf(toFind);
        if (i != -1) {
            _boldActivated = true;
        } else {
            _boldActivated = false;
        }

        _setBoldButton->setChecked(_boldActivated);
        _setBoldButton->setDown(_boldActivated);

        QString fontSizeString;
        QString fontColorString;
        toFind = QString(kFontSizeTag);
        i = text.indexOf(toFind);
        bool foundFontTag = false;
        if (i != -1) {
            foundFontTag = true;
            i += toFind.size();
            while ( i < text.size() && text.at(i).isDigit() ) {
                fontSizeString.append( text.at(i) );
                ++i;
            }
        }
        toFind = QString(kFontColorTag);
        i = text.indexOf(toFind,i);
        assert( (!foundFontTag && i == -1) || (foundFontTag && i != -1) );
        if (i != -1) {
            i += toFind.size();
            while ( i < text.size() && text.at(i) != QChar('"') ) {
                fontColorString.append( text.at(i) );
                ++i;
            }
        }
        toFind = QString(kFontFaceTag);
        i = text.indexOf(toFind,i);
        assert( (!foundFontTag && i == -1) || (foundFontTag && i != -1) );
        if (i != -1) {
            i += toFind.size();
            while ( i < text.size() && text.at(i) != QChar('"') ) {
                _fontFamily.append( text.at(i) );
                ++i;
            }
        }

        if (!foundFontTag) {
            _fontSize = _fontSizeSpinBox->value();
            _fontColor = Qt::black;
            _fontFamily = _fontCombo->currentFont().family();
            _boldActivated = false;
            _italicActivated = false;
            QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);
            text.prepend(fontTag);
            text.append(kFontEndTag);
            knob->setValue(text.toStdString());
        } else {
            _fontCombo->setCurrentFont( QFont(_fontFamily) );

            _fontSize = fontSizeString.toInt();

            _fontSizeSpinBox->setValue(_fontSize);

            _fontColor = QColor(fontColorString);
        }


        updateFontColorIcon(_fontColor);
    }
} // restoreTextInfoFromString

void
KnobGuiString::parseFont(const QString & label,
                          QFont *f,
                          QColor *color)
{
    QString toFind = QString(kFontSizeTag);
    int startFontTag = label.indexOf(toFind);

    assert(startFontTag != -1);
    startFontTag += toFind.size();
    int j = startFontTag;
    QString sizeStr;
    while ( j < label.size() && label.at(j).isDigit() ) {
        sizeStr.push_back( label.at(j) );
        ++j;
    }

    toFind = QString(kFontFaceTag);
    startFontTag = label.indexOf(toFind,startFontTag);
    assert(startFontTag != -1);
    startFontTag += toFind.size();
    j = startFontTag;
    QString faceStr;
    while ( j < label.size() && label.at(j) != QChar('"') ) {
        faceStr.push_back( label.at(j) );
        ++j;
    }

    f->setPointSize( sizeStr.toInt() );
    f->setFamily(faceStr);
    
    {
        toFind = QString(kBoldStartTag);
        int foundBold = label.indexOf(toFind);
        if (foundBold != -1) {
            f->setBold(true);
        }
    }
    
    {
        toFind = QString(kItalicStartTag);
        int foundItalic = label.indexOf(toFind);
        if (foundItalic != -1) {
            f->setItalic(true);
        }
    }
    {
        toFind = QString(kFontColorTag);
        int foundColor = label.indexOf(toFind);
        if (foundColor != -1) {
            foundColor += toFind.size();
            QString currentColor;
            int j = foundColor;
            while ( j < label.size() && label.at(j) != QChar('"') ) {
                currentColor.push_back( label.at(j) );
                ++j;
            }
            *color = QColor(currentColor);
        }
    }
}

void
KnobGuiString::updateFontColorIcon(const QColor & color)
{
    QPixmap p(18,18);

    p.fill(color);
    _fontColorButton->setIcon( QIcon(p) );
}

void
KnobGuiString::onCurrentFontChanged(const QFont & font)
{
    
    boost::shared_ptr<KnobString> knob = _knob.lock();
    assert(_textEdit);
    QString text( knob->getValue(0).c_str() );
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    _fontFamily = font.family();
    if (i != -1) {
        toFind = QString(kFontFaceTag);
        i = text.indexOf(toFind,i);
        assert(i != -1);
        i += toFind.size();
        ///erase the current font face (family)
        QString currentFontFace;
        int j = i;
        while ( j < text.size() && text.at(j) != QChar('"') ) {
            currentFontFace.push_back( text.at(j) );
            ++j;
        }
        text.remove( i, currentFontFace.size() );
        text.insert( i != -1 ? i : 0, font.family() );
    } else {
        QString fontTag = makeFontTag(_fontFamily,_fontSize,_fontColor);
        text.prepend(fontTag);
        text.append(kFontEndTag);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getValue(0),text.toStdString() ) );
}

QString
KnobGuiString::makeFontTag(const QString& family,int fontSize,const QColor& color)
{
    return QString(kFontSizeTag "%1\" " kFontColorTag "%2\" " kFontFaceTag "%3\">")
    .arg(fontSize)
    .arg( color.name() )
    .arg(family);
}

QString
KnobGuiString::decorateTextWithFontTag(const QString& family,int fontSize,const QColor& color,const QString& text)
{
    return makeFontTag(family, fontSize, color) + text + kFontEndTag;
}

void
KnobGuiString::onFontSizeChanged(double size)
{
    assert(_textEdit);
    boost::shared_ptr<KnobString> knob = _knob.lock();;
    QString text( knob->getValue(0).c_str() );
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    assert(i != -1);
    i += toFind.size();
    ///erase the current font face (family)
    QString currentSize;
    int j = i;
    while ( j < text.size() && text.at(j).isDigit() ) {
        currentSize.push_back( text.at(j) );
        ++j;
    }
    text.remove( i, currentSize.size() );
    text.insert( i, QString::number(size) );
    _fontSize = size;
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getValue(0),text.toStdString() ) );
}

void
KnobGuiString::boldChanged(bool toggled)
{
    assert(_textEdit);
    boost::shared_ptr<KnobString> knob = _knob.lock();
    QString text( knob->getValue(0).c_str() );
    QString toFind = QString(kBoldStartTag);
    int i = text.indexOf(toFind);

    assert( (toggled && i == -1) || (!toggled && i != -1) );

    if (!toggled) {
        text.remove( i, toFind.size() );
        toFind = QString(kBoldEndTag);
        i = text.lastIndexOf(toFind);
        assert (i != -1);
        text.remove( i,toFind.size() );
    } else {
        ///insert right prior to the font size
        toFind = QString(kFontSizeTag);
        i = text.indexOf(toFind);
        assert(i != -1);
        text.insert(i, kBoldStartTag);
        toFind = QString(kFontEndTag);
        i = text.lastIndexOf(toFind);
        assert(i != -1);
        i += toFind.size();
        text.insert(i, kBoldEndTag);
    }

    _boldActivated = toggled;
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getValue(0),text.toStdString() ) );
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
    boost::shared_ptr<KnobString> knob = _knob.lock();
    QColorDialog dialog(_textEdit);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( updateFontColorIcon(QColor) ) );
    dialog.setCurrentColor(_fontColor);
    if ( dialog.exec() ) {
        _fontColor = dialog.currentColor();

        QString text( knob->getValue(0).c_str() );
        findReplaceColorName(text,_fontColor.name());
        pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getValue(0),text.toStdString() ) );
    }
    updateFontColorIcon(_fontColor);
}

void
KnobGuiString::findReplaceColorName(QString& text,const QColor& color)
{
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    if (i != -1) {
        toFind = QString(kFontColorTag);
        int foundColorTag = text.indexOf(toFind,i);
        if (foundColorTag != -1) {
            foundColorTag += toFind.size();
            QString currentColor;
            int j = foundColorTag;
            while ( j < text.size() && text.at(j) != QChar('"') ) {
                currentColor.push_back( text.at(j) );
                ++j;
            }
            text.remove( foundColorTag,currentColor.size() );
            text.insert( foundColorTag, color.name() );
        } else {
            text.insert(i, kFontColorTag);
            text.insert(i + toFind.size(), color.name() + "\"");
        }
    }
    
    
}

void
KnobGuiString::italicChanged(bool toggled)
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
    QString text( knob->getValue(0).c_str() );
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);

    assert(i != -1);

    ///search before i
    toFind = QString(kItalicStartTag);
    int foundItalic = text.lastIndexOf(toFind,i);
    assert( (toggled && foundItalic == -1) || (!toggled && foundItalic != -1) );
    if (!toggled) {
        text.remove( foundItalic, toFind.size() );
        toFind = QString(kItalicEndTag);
        foundItalic = text.lastIndexOf(toFind);
        assert(foundItalic != -1);
        text.remove( foundItalic, toFind.size() );
    } else {
        int foundBold = text.lastIndexOf(kBoldStartTag,i);
        assert( (foundBold == -1 && !_boldActivated) || (foundBold != -1 && _boldActivated) );

        ///if bold is activated, insert prior to bold
        if (foundBold != -1) {
            foundBold = foundBold == 0 ? 0 : foundBold - 1;
            text.insert(foundBold, kItalicStartTag);
        } else {
            //there's no bold
            i = i == 0 ? 0 : i - 1;
            text.insert(i, kItalicStartTag);
        }
        text.append(kItalicEndTag); //< this is always the last tag
    }
    _italicActivated = toggled;
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getValue(0),text.toStdString() ) );
}

QString
KnobGuiString::removeNatronHtmlTag(QString text)
{
    ///we also remove any custom data added by natron so the user doesn't see it
    int startCustomData = text.indexOf(NATRON_CUSTOM_HTML_TAG_START);

    if (startCustomData != -1) {
        QString endTag(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = text.indexOf(endTag,startCustomData);
        assert(endCustomData != -1);
        endCustomData += endTag.size();
        text.remove(startCustomData, endCustomData - startCustomData);
    }

    return text;
}

QString
KnobGuiString::getNatronHtmlTagContent(QString text)
{
    QString label = removeAutoAddedHtmlTags(text,false);
    QString startTag(NATRON_CUSTOM_HTML_TAG_START);
    int startCustomData = label.indexOf(startTag);
    if (startCustomData != -1) {
        QString endTag(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = label.indexOf(endTag,startCustomData);
        assert(endCustomData != -1);
        label = label.remove(endCustomData, endTag.size());
        label = label.remove(startCustomData, startTag.size());
    }
    return label;
}


QString
KnobGuiString::removeAutoAddedHtmlTags(QString text,bool removeNatronTag)
{
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    bool foundFontStart = i != -1;
    QString boldStr(kBoldStartTag);
    int foundBold = text.lastIndexOf(boldStr,i);

    ///Assert removed: the knob might be linked from elsewhere and the button might not have been pressed.
    //assert((foundBold == -1 && !_boldActivated) || (foundBold != -1 && _boldActivated));

    if (foundBold != -1) {
        text.remove( foundBold, boldStr.size() );
        boldStr = QString(kBoldEndTag);
        foundBold = text.lastIndexOf(boldStr);
        assert(foundBold != -1);
        text.remove( foundBold,boldStr.size() );
    }

    ///refresh the index
    i = text.indexOf(toFind);

    QString italStr(kItalicStartTag);
    int foundItal = text.lastIndexOf(italStr,i);

    //Assert removed: the knob might be linked from elsewhere and the button might not have been pressed.
    // assert((_italicActivated && foundItal != -1) || (!_italicActivated && foundItal == -1));

    if (foundItal != -1) {
        text.remove( foundItal, italStr.size() );
        italStr = QString(kItalicEndTag);
        foundItal = text.lastIndexOf(italStr);
        assert(foundItal != -1);
        text.remove( foundItal,italStr.size() );
    }

    ///refresh the index
    i = text.indexOf(toFind);

    QString endTag("\">");
    int foundEndTag = text.indexOf(endTag,i);
    foundEndTag += endTag.size();
    if (foundFontStart) {
        ///remove the whole font tag
        text.remove(i,foundEndTag - i);
    }

    endTag = QString(kFontEndTag);
    foundEndTag = text.lastIndexOf(endTag);
    assert( (foundEndTag != -1 && foundFontStart) || !foundFontStart );
    if (foundEndTag != -1) {
        text.remove( foundEndTag, endTag.size() );
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
    boost::shared_ptr<KnobString> knob = _knob.lock();
    std::string value = knob->getValue(0);

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->blockSignals(true);
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();
        QString txt( value.c_str() );
        if (_knob.lock()->usesRichText()) {
            txt = removeAutoAddedHtmlTags(txt);
        }

        if (knob->isCustomHTMLText()) {
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
        cursor.setPosition(selectionEnd,QTextCursor::KeepAnchor);

        _textEdit->setTextCursor(cursor);
        _textEdit->blockSignals(false);
    } else if ( knob->isLabel() ) {
        onLabelChanged();
        /*assert(_label);
        QString txt = value.c_str();
        txt.replace("\n", "<br>");
        _label->setText(txt);*/
    } else {
        assert(_lineEdit);
        _lineEdit->setText( value.c_str() );
    }
}

void
KnobGuiString::_hide()
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->hide();
    } else if ( knob->isLabel() ) {
        /*assert(_label);
        _label->hide();*/
    } else {
        assert(_lineEdit);
        _lineEdit->hide();
    }
}

void
KnobGuiString::_show()
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->show();
    } else if ( knob->isLabel() ) {
       /* assert(_label);
        _label->show();*/
    } else {
        assert(_lineEdit);
        _lineEdit->show();
    }
}

void
KnobGuiString::setEnabled()
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
    bool b = knob->isEnabled(0)  &&  knob->getExpression(0).empty();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        //_textEdit->setEnabled(b);
        //_textEdit->setReadOnly(!b);
        if (!knob->isCustomHTMLText()) {
            _textEdit->setReadOnlyNatron(!b);
        }
    } else if ( knob->isLabel() ) {
       /* assert(_label);
        _label->setEnabled(b);*/
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        if ( !knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(!b);
        }
    }
}

void
KnobGuiString::reflectAnimationLevel(int /*dimension*/,
                                      AnimationLevelEnum level)
{
    boost::shared_ptr<KnobString> knob = _knob.lock();
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
    if (_textEdit) {
        if (!_knob.lock()->isCustomHTMLText()) {
            _textEdit->setReadOnlyNatron(readOnly);
        }
    } else if (_lineEdit) {
        if ( !_knob.lock()->isCustomKnob() ) {
            _lineEdit->setReadOnly(readOnly);
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

KnobPtr
KnobGuiString::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiString::reflectExpressionState(int /*dimension*/,
                                       bool hasExpr)
{
    bool isEnabled = _knob.lock()->isEnabled(0);
    if (_textEdit) {
        _textEdit->setAnimation(3);
        if (!_knob.lock()->isCustomHTMLText()) {
            _textEdit->setReadOnlyNatron(hasExpr || !isEnabled);
        }
    } else if (_lineEdit) {
        _lineEdit->setAnimation(3);
        _lineEdit->setReadOnly(hasExpr || !isEnabled);
    }
}

void
KnobGuiString::updateToolTip()
{
    if (hasToolTip()) {
        QString tt = toolTip();
        if (_textEdit) {
            bool useRichText = _knob.lock()->usesRichText();
            if (useRichText) {
                tt += tr(" This text area supports html encoding. "
                         "Please check <a href=http://qt-project.org/doc/qt-5/richtext-html-subset.html>Qt website</a> for more info. ");
            }
            QKeySequence seq(Qt::CTRL + Qt::Key_Return);
            tt += tr("Use ") + seq.toString(QKeySequence::NativeText) + tr(" to validate changes made to the text. ");
            _textEdit->setToolTip(tt);
        } else if (_lineEdit) {
            _lineEdit->setToolTip(tt);
        }
        //else if (_label) {
           // _label->setToolTip(tt);
        //}
    }
}

void
KnobGuiString::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();
    if (_lineEdit) {
        _lineEdit->setAltered(!hasModif);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiString.cpp"

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/KnobUndoCommand.h"
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
#include "Gui/KnobGui.h"
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



AnimatingTextEdit::AnimatingTextEdit(const KnobGuiPtr& knob,
                                     DimSpec dimension,
                                     ViewIdx view,
                                     QWidget* parent)
    : QTextEdit(parent)
    , StyledKnobWidgetBase()
    , _hasChanged(false)
    , _dnd( KnobWidgetDnD::create(knob, dimension, view, this) )
{
    setTabStopWidth(20); // a tab width of 20 is more reasonable than 80 for programming languages (e.g. Shadertoy)
}

AnimatingTextEdit::~AnimatingTextEdit()
{
}

void
AnimatingTextEdit::setLinkedAppearanceEnabled(bool linked)
{
    _appearLinked = linked;
    refreshStylesheet();
}

void
AnimatingTextEdit::refreshStylesheet()
{

    double bgColor[3];
    bool bgColorSet = false;

    if (multipleSelection) {
        bgColor[0] = bgColor[1] = bgColor[2] = 0;
        bgColorSet = true;
    }

    if (!bgColorSet) {
        // draw the background with
        // a color reflecting the animation level
        switch ((AnimationLevelEnum)animation) {
            case eAnimationLevelExpression:
                appPTR->getCurrentSettings()->getExprColor(&bgColor[0], &bgColor[1], &bgColor[2]);
                bgColorSet = true;
                break;
            case eAnimationLevelInterpolatedValue:
                appPTR->getCurrentSettings()->getInterpolatedColor(&bgColor[0], &bgColor[1], &bgColor[2]);
                bgColorSet = true;
                break;
            case eAnimationLevelOnKeyframe:
                appPTR->getCurrentSettings()->getKeyframeColor(&bgColor[0], &bgColor[1], &bgColor[2]);
                bgColorSet = true;
                break;
            case eAnimationLevelNone:
                break;
        }
    }

    double fgColor[3];

    if (!isEnabled() || isReadOnly() || (AnimationLevelEnum)animation == eAnimationLevelExpression) {
        fgColor[0] = fgColor[1] = fgColor[2] = 0.;
    } else if (_appearLinked) {
        appPTR->getCurrentSettings()->getExprColor(&fgColor[0], &fgColor[1], &fgColor[2]);
    } else {
        appPTR->getCurrentSettings()->getTextColor(&fgColor[0], &fgColor[1], &fgColor[2]);
    }

    QColor bgCol;
    if (bgColorSet) {
        bgCol.setRgbF(Image::clamp(bgColor[0], 0., 1.), Image::clamp(bgColor[1], 0., 1.), Image::clamp(bgColor[2], 0., 1.));
    }


    QColor fgCol;
    fgCol.setRgbF(Image::clamp(fgColor[0], 0., 1.), Image::clamp(fgColor[1], 0., 1.), Image::clamp(fgColor[2], 0., 1.));

    if (bgCol == _lastBgColor && fgCol == _lastFgColor) {
        return;
    }

    _lastBgColor = bgCol;
    _lastFgColor = fgCol;

    QString bgColorStyleSheetStr;
    if (bgColorSet) {
        bgColorStyleSheetStr = QString::fromUtf8("background-color: rgb(%1, %2, %3);").arg(bgCol.red()).arg(bgCol.green()).arg(bgCol.blue())
        ;
    }


    setStyleSheet(QString::fromUtf8("QTextEdit {\n"
                                    "color: rgb(%1, %2, %3);\n"
                                    "%4\n"
                                    "}\n").arg(fgCol.red()).arg(fgCol.green()).arg(fgCol.blue()).arg(bgColorStyleSheetStr));




    style()->unpolish(this);
    style()->polish(this);
    update();
}


void
AnimatingTextEdit::enterEvent(QEvent* e)
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
    if (e->key() != Qt::Key_Control &&
        e->key() != Qt::Key_Shift &&
        e->key() != Qt::Key_Alt) {
        _hasChanged = true;
    }
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
                           DimSpec dimension,
                           ViewIdx view,
                           QWidget* parent)
    : LineEdit(parent)
    , _dnd( KnobWidgetDnD::create(knob, dimension, view, this) )
{}

KnobLineEdit::~KnobLineEdit()
{
}

void
KnobLineEdit::enterEvent(QEvent* e)
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

KnobGuiString::KnobGuiString(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
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
{
    _knob = toKnobString(knob->getKnob());
}

QFont
KnobGuiString::makeFontFromState() const
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return QFont();
    }
    QFont f;
    f.setFamily(QString::fromUtf8(knob->getFontFamily().c_str()));
    f.setPointSize(knob->getFontSize());
    f.setBold(knob->getBoldActivated());
    f.setItalic(knob->getItalicActivated());
    return f;
}

void
KnobGuiString::createWidget(QHBoxLayout* layout)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    connect(this, SIGNAL(fontPropertyChanged()),this,SLOT(onFontPropertyChanged()));
    KnobGuiPtr knobUI = getKnobGui();
    if ( knob->isMultiLine() ) {
        _container = new QWidget( layout->parentWidget() );
        _mainLayout = new QVBoxLayout(_container);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);

        bool useRichText = knob->usesRichText();
        _textEdit = new AnimatingTextEdit(knobUI, DimIdx(0), getView(), _container);
        _textEdit->setAcceptRichText(useRichText);


        _mainLayout->addWidget(_textEdit);

        QObject::connect( _textEdit, SIGNAL(editingFinished()), this, SLOT(onTextChanged()) );
        // layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        ///set the copy/link actions in the right click menu
        KnobGuiWidgets::enableRightClickMenu(knobUI, _textEdit, DimIdx(0), getView());

        if ( knob->isCustomHTMLText() ) {
            _textEdit->setEnabled(false);
        }

        if (useRichText) {
            _richTextOptions = new QWidget(_container);
            _richTextOptionsLayout = new QHBoxLayout(_richTextOptions);
            _richTextOptionsLayout->setContentsMargins(0, 0, 0, 0);
            _richTextOptionsLayout->setSpacing(TO_DPIX(8));

            _fontCombo = new QFontComboBox(_richTextOptions);
            _fontCombo->setCurrentFont( makeFontFromState() );
            _fontCombo->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _richTextOptionsLayout->addWidget(_fontCombo);

            _fontSizeSpinBox = new SpinBox(_richTextOptions);
            _fontSizeSpinBox->setMinimum(1);
            _fontSizeSpinBox->setMaximum(100);
            _fontSizeSpinBox->setValue(knob->getFontSize());
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
            _setBoldButton->setChecked(knob->getBoldActivated());
            _setBoldButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Bold."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _setBoldButton->setMaximumSize(18, 18);
            QObject::connect( _setBoldButton, SIGNAL(clicked(bool)), this, SLOT(boldChanged(bool)) );
            _richTextOptionsLayout->addWidget(_setBoldButton);

            QIcon italicIcon;
            italicIcon.addPixmap(pixItalicChecked, QIcon::Normal, QIcon::On);
            italicIcon.addPixmap(pixItalicUnchecked, QIcon::Normal, QIcon::Off);

            _setItalicButton = new Button(italicIcon, QString(), _richTextOptions);
            _setItalicButton->setCheckable(true);
            _setItalicButton->setChecked(knob->getItalicActivated());
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

            double r,g,b;
            knob->getFontColor(&r, &g, &b);
            QColor c;
            c.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
            updateFontColorIcon(c);
            _richTextOptionsLayout->addStretch();

            _mainLayout->addWidget(_richTextOptions);


            ///Connect the slot after restoring
            QObject::connect( _fontCombo, SIGNAL(currentFontChanged(QFont)), this, SLOT(onCurrentFontChanged(QFont)) );
        }

        layout->addWidget(_container);
    } else if ( knob->isLabel() ) {
        const std::string& iconFilePath = knob->getIconLabel();
        if ( !iconFilePath.empty() ) {
            _label = new Label( layout->parentWidget() );

            if ( knobUI->hasToolTip() ) {
                knobUI->toolTip(_label, getView());
            }
            layout->addWidget(_label);
        }
    } else {
        _lineEdit = new KnobLineEdit( knobUI, DimIdx(0), getView(), layout->parentWidget() );

        if ( knobUI->hasToolTip() ) {
            knobUI->toolTip(_lineEdit, getView());
        }
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(_lineEdit);
        QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onLineChanged()) );

        if ( knob->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(true);
        }

        ///set the copy/link actions in the right click menu
        KnobGuiWidgets::enableRightClickMenu(knobUI, _lineEdit, DimIdx(0), getView());
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
KnobGuiString::onFontPropertyChanged()
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->evaluateValueChange(DimSpec(0), knob->getCurrentRenderTime(), ViewIdx(0), eValueChangedReasonUserEdited);
}

bool
KnobGuiString::shouldAddStretch() const
{
    // Never add stretch after the line edits, text edit, they expand horizontally
    return _label != 0;
}

bool
KnobGuiString::parseFont(const QString & s, QFont* f, QColor* color)
{
    double r,g,b;
    QString family;
    bool bold;
    bool italic;
    int size;
    if (!KnobString::parseFont(s, &size, &family, &bold, &italic, &r, &g, &b)) {
        return false;
    }
    f->setFamily(family);
    f->setPointSize(size);
    f->setBold(bold);
    f->setItalic(italic);
    color->setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
    return true;
}


std::string
KnobGuiString::getDescriptionLabel() const
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return "";
    }
    bool isLabel = knob->isLabel();

    if (isLabel) {
        return knob->getValue();
    } else {
        return knob->getLabel();
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
    std::string oldText = knob->getValue(DimIdx(0), getView());
    std::string newText = _lineEdit->text().toStdString();

    if (oldText != newText) {
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldText, newText, DimIdx(0), getView()) );
    }

}

void
KnobGuiString::onTextChanged()
{
    QString txt = _textEdit->toPlainText();
    KnobGuiPtr knobUI = getKnobGui();
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string oldText = knob->getValue(DimIdx(0), getView());
    knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldText, txt.toStdString(), DimIdx(0), getView() ) );
}


void
KnobGuiString::updateFontColorIcon(const QColor & color)
{
    QPixmap p(18, 18);

    p.fill(color);
    _fontColorButton->setIcon( QIcon(p) );
}

void
KnobGuiString::onCurrentFontChanged(const QFont & font)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setFontFamily(font.family().toStdString());
    updateGUI();
    Q_EMIT fontPropertyChanged();
}



void
KnobGuiString::onFontSizeChanged(double size)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setFontSize(size);
    updateGUI();
    Q_EMIT fontPropertyChanged();
}

void
KnobGuiString::boldChanged(bool toggled)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setBoldActivated(toggled);
    updateGUI();
    Q_EMIT fontPropertyChanged();
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


    QColorDialog dialog(_textEdit);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(updateFontColorIcon(QColor)) );

    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    QColor currentColor;
    {
        double r,g,b;
        knob->getFontColor(&r, &g, &b);
        currentColor.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
    }
    dialog.setCurrentColor(currentColor);
    if ( dialog.exec() ) {
        currentColor = dialog.currentColor();
        double r = currentColor.redF();
        double g = currentColor.greenF();
        double b = currentColor.blueF();
        knob->setFontColor(r, g, b);
        updateGUI();
        Q_EMIT fontPropertyChanged();

    }
    updateFontColorIcon(currentColor);
}


void
KnobGuiString::italicChanged(bool toggled)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setItalicActivated(toggled);
    updateGUI();
    Q_EMIT fontPropertyChanged();
}



void
KnobGuiString::updateGUI()
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string value = knob->getValue(DimIdx(0), getView());

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();

        QString txt = QString::fromUtf8( value.c_str() );

        if ( knob->isCustomHTMLText() ) {
            QString oldText = _textEdit->toHtml();
            if (oldText == txt) {
                return;
            }
            _textEdit->blockSignals(true);
            _textEdit->setHtml(txt);
        } else {
            QString oldText = _textEdit->toPlainText();
            if (oldText == txt) {
                return;
            }
            _textEdit->blockSignals(true);
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
            if (_label->text() == txt) {
                return;
            }
            _label->setText(txt);
        }
    } else {
        assert(_lineEdit);
        QString text = QString::fromUtf8( value.c_str() );
        if (_lineEdit->text() == text) {
            return;
        }
        _lineEdit->setText(text);
    }
} // KnobGuiString::updateGUI

void
KnobGuiString::setWidgetsVisible(bool visible)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->setVisible(visible);
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->setVisible(visible);
        }
    } else {
        assert(_lineEdit);
        _lineEdit->setVisible(visible);
    }
}


void
KnobGuiString::setEnabled(const std::vector<bool>& perDimEnabled)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        if ( !knob->isCustomHTMLText() ) {
            _textEdit->setEnabled(perDimEnabled[0]);
        }
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->setEnabled(perDimEnabled[0]);
        }
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        if ( !knob->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(!perDimEnabled[0]);
        }
    }
}

void
KnobGuiString::reflectLinkedState(DimIdx /*dimension*/, bool linked)
{
    if (_lineEdit) {
        QColor c;
        if (linked) {
            double r,g,b;
            appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
            c.setRgbF(Image::clamp(r, 0., 1.),
                      Image::clamp(g, 0., 1.),
                      Image::clamp(b, 0., 1.));
            _lineEdit->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, true, c);
        } else {
            _lineEdit->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, false);
        }
    } else if (_textEdit) {
        _textEdit->setLinkedAppearanceEnabled(linked);
    }
}

void
KnobGuiString::reflectAnimationLevel(DimIdx /*dimension*/,
                                     AnimationLevelEnum level)
{
    KnobStringPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool isEnabled = knob->isEnabled();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->setAnimation(level);
        _textEdit->setEnabled(level != eAnimationLevelExpression && isEnabled);
    } else if ( knob->isLabel() ) {
        //assert(_label);
    } else {
        assert(_lineEdit);
        _lineEdit->setAnimation(level);
        _lineEdit->setReadOnly_NoFocusRect(level == eAnimationLevelExpression || !isEnabled);

    }
}

void
KnobGuiString::reflectMultipleSelection(bool dirty)
{
    if (_textEdit) {
        _textEdit->setIsSelectedMultipleTimes(dirty);
    } else if (_lineEdit) {
        _lineEdit->setIsSelectedMultipleTimes(dirty);
    }
}

void
KnobGuiString::reflectSelectionState(bool selected)
{
    if (_textEdit) {
        _textEdit->setIsSelected(selected);
    } else if (_lineEdit) {
        _lineEdit->setIsSelected(selected);
    }

}


void
KnobGuiString::updateToolTip()
{
    KnobGuiPtr knobUI = getKnobGui();
    if ( knobUI && knobUI->hasToolTip() ) {

        if (_textEdit) {
            KnobStringPtr knob = _knob.lock();
            if (!knob) {
                return;
            }
            bool useRichText = knob->usesRichText();
            QString tt = knobUI->toolTip(0, getView());
            if (useRichText) {
                tt += tr("This text area supports html encoding. "
                         "Please check <a href=http://qt-project.org/doc/qt-5/richtext-html-subset.html>Qt website</a> for more info.");
            }
            QKeySequence seq(Qt::CTRL + Qt::Key_Return);
            tt += tr("Use %1 to validate changes made to the text.").arg( seq.toString(QKeySequence::NativeText) );
            _textEdit->setToolTip(tt);
        } else if (_lineEdit) {
            knobUI->toolTip(_lineEdit, getView());
        } else if (_label) {
            knobUI->toolTip(_label, getView());
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
        _lineEdit->setIsModified(hasModif);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiString.cpp"

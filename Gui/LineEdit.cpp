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

#include "LineEdit.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QLineEdit>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QDragEnterEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPainter>
#include <QKeySequence>
#include <QtCore/QUrl>
#include <QtCore/QMimeData>
#include <QStyle>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/QtCompat.h"

#include "Engine/Image.h"
#include "Engine/Settings.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"

NATRON_NAMESPACE_ENTER

LineEdit::LineEdit(QWidget* parent)
    : QLineEdit(parent)
    , StyledKnobWidgetBase()
    , _customColor()
    , _customColorSet(false)
    , isBold(false)
    , borderDisabled(false)
{
    setAttribute(Qt::WA_MacShowFocusRect, 0);
    setFixedHeight( TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
}

LineEdit::~LineEdit()
{
}

void
LineEdit::dropEvent(QDropEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QList<QUrl> urls = e->mimeData()->urls();
    QString path;
    if (urls.size() > 0) {
        path = QtCompat::toLocalFileUrlFixed( urls.at(0) ).toLocalFile();
    }
    if ( !path.isEmpty() ) {
        setText(path);
        selectAll();
        setFocus( Qt::MouseFocusReason );
        e->acceptProposedAction();
        update();
        Q_EMIT textDropped();
    }
}


void
LineEdit::dragEnterEvent(QDragEnterEvent* e)
{
    e->acceptProposedAction();
}

void
LineEdit::dragMoveEvent(QDragMoveEvent* e)
{
    e->acceptProposedAction();
}

void
LineEdit::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

bool
LineEdit::getIsBold() const
{
    return isBold;
}

void
LineEdit::setIsBold(bool b)
{
    isBold = b;
    refreshStylesheet();
}

void
LineEdit::refreshStylesheet()
{
    double bgColor[3];
    bool bgColorSet = false;

    if (multipleSelection) {
        bgColor[0] = bgColor[1] = bgColor[2] = 0;
        bgColorSet = true;
    }
    if (!bgColorSet && !borderDisabled) {
        // If border is not disabled, draw the background with
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
    bool fgColorSet = false;
    if (!isEnabled() || isReadOnly() || (AnimationLevelEnum)animation == eAnimationLevelExpression) {
        fgColor[0] = fgColor[1] = fgColor[2] = 0.;
        fgColorSet = true;
    }
    if (!fgColorSet) {
        if (_customColorSet) {
            fgColor[0] = _customColor.redF();
            fgColor[1] = _customColor.greenF();
            fgColor[2] = _customColor.blueF();
            fgColorSet = true;
        }
    }
    if (!fgColorSet) {
        if (selected) {
            appPTR->getCurrentSettings()->getSelectionColor(&fgColor[0], &fgColor[1], &fgColor[2]);
            fgColorSet = true;
        }
    }
    if (!fgColorSet && borderDisabled) {
        // When border is disabled, reflect the animation level on the text color instead of the
        // background
        switch ((AnimationLevelEnum)animation) {
            case eAnimationLevelExpression:
                appPTR->getCurrentSettings()->getExprColor(&bgColor[0], &fgColor[1], &fgColor[2]);
                fgColorSet = true;
                break;
            case eAnimationLevelInterpolatedValue:
                appPTR->getCurrentSettings()->getInterpolatedColor(&fgColor[0], &fgColor[1], &fgColor[2]);
                fgColorSet = true;
                break;
            case eAnimationLevelOnKeyframe:
                appPTR->getCurrentSettings()->getKeyframeColor(&fgColor[0], &fgColor[1], &fgColor[2]);
                fgColorSet = true;
                break;
            case eAnimationLevelNone:
                break;
        }

    }
    if (!fgColorSet) {
        if (!getIsModified()) {
            appPTR->getCurrentSettings()->getAltTextColor(&fgColor[0], &fgColor[1], &fgColor[2]);
        } else {
            appPTR->getCurrentSettings()->getTextColor(&fgColor[0], &fgColor[1], &fgColor[2]);
        }
    }
    QColor fgCol;
    fgCol.setRgbF(Image::clamp(fgColor[0], 0., 1.), Image::clamp(fgColor[1], 0., 1.), Image::clamp(fgColor[2], 0., 1.));

    QString bgColorStyleSheetStr;
    if (bgColorSet) {
        QColor bgCol;
        bgCol.setRgbF(Image::clamp(bgColor[0], 0., 1.), Image::clamp(bgColor[1], 0., 1.), Image::clamp(bgColor[2], 0., 1.));
        bgColorStyleSheetStr = QString::fromUtf8("background-color: rgb(%1, %2, %3);").arg(bgCol.red()).arg(bgCol.green()).arg(bgCol.blue())
;
    }
    setStyleSheet(QString::fromUtf8("QLineEdit {\n"
                                    "color: rgb(%1, %2, %3);\n"
                                    "%4\n"
                                    "%5\n"
                                    "}\n").arg(fgCol.red()).arg(fgCol.green()).arg(fgCol.blue())
                  .arg(bgColorStyleSheetStr)
                  .arg(isBold ? QString::fromUtf8("font-weight: bold;") : QString()));


    style()->unpolish(this);
    style()->polish(this);
    update();
}


void
LineEdit::setReadOnly(bool ro)
{
    // Should never be called since on Mac is set the WA_ShowFocusRect attribute to 1 which
    // makes the application UI redraw all its widgets for any change.
    assert(false);
    QLineEdit::setReadOnly(ro);
}

void
LineEdit::setReadOnly_NoFocusRect(bool readOnly)
{
    QLineEdit::setReadOnly(readOnly);

    // setReadonly set the flag but on mac a bug makes
    // it redraw the whole UI and slow down the software
    setAttribute(Qt::WA_MacShowFocusRect, 0);
    refreshStylesheet();
}

void
LineEdit::setCustomTextColor(const QColor& color)
{
    _customColorSet = true;
    if (color != _customColor) {
        _customColor = color;
        refreshStylesheet();
    }
}


void
LineEdit::disableAllDecorations()
{
    decorationType.clear();
    update();
}

void
LineEdit::setAdditionalDecorationTypeEnabled(AdditionalDecorationType type, bool enabled, const QColor& color) {
    AdditionalDecoration& deco = decorationType[type];
    deco.enabled = enabled;
    deco.color = color;
    update();
}

void
LineEdit::setBorderDisabled(bool disabled)
{
    borderDisabled = disabled;
    refreshStylesheet();
}

bool
LineEdit::getBorderDisabled() const
{
    return borderDisabled;
}

void
LineEdit::paintEvent(QPaintEvent *e)
{
    QLineEdit::paintEvent(e);

    for (AdditionalDecorationsMap::iterator it = decorationType.begin(); it!=decorationType.end(); ++it) {
        if (!it->second.enabled) {
            continue;
        }
        switch (it->first) {
            case eAdditionalDecorationColoredFrame:
            {
                QPainter p(this);
                p.setPen(it->second.color);
                QRect bRect = rect();
                bRect.adjust(0, 0, -1, -1);
                p.drawRect(bRect);
            }   break;
            case eAdditionalDecorationColoredUnderlinedText:
            {
                QPainter p(this);
                p.setPen(it->second.color);
                int h = height() - 1;
                p.drawLine(0, h - 1, width() - 1, h - 1);
            }   break;
        }
    }
}

void
LineEdit::keyPressEvent(QKeyEvent* e)
{
    QLineEdit::keyPressEvent(e);

    if ( e->matches(QKeySequence::Paste) ) {
        Q_EMIT textPasted();
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_LineEdit.cpp"

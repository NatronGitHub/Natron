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

#include "Label.h"

#include <stdexcept>

#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QColor>

#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Gui/GuiApplicationManager.h"


NATRON_NAMESPACE_ENTER

void
Label::init()
{
    isBold = false;
    _customColorSet = false;
    setFont( QApplication::font() ); // necessary, or the labels will get the default font size
}

Label::Label(const QString &text,
             QWidget *parent,
             Qt::WindowFlags f)
    : QLabel(text, parent, f)
    , StyledKnobWidgetBase()

{
    init();
}

Label::Label(QWidget *parent,
             Qt::WindowFlags f)
    : QLabel(parent, f)
    , StyledKnobWidgetBase()
{
    init();
}

void
Label::setCustomTextColor(const QColor& color)
{
    _customColorSet = true;
    if (color != _customColor) {
        _customColor = color;
        refreshStylesheet();
    }
}

bool
Label::getIsBold() const
{
    return isBold;
}

void
Label::setIsBold(bool b)
{
    isBold = b;
    refreshStylesheet();
}

void
Label::refreshStylesheet()
{

    double fgColor[3];
    bool fgColorSet = false;
    if (!isEnabled()) {
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
    if (!fgColorSet) {

        if (!getIsModified()) {
            appPTR->getCurrentSettings()->getAltTextColor(&fgColor[0], &fgColor[1], &fgColor[2]);
        } else {
            appPTR->getCurrentSettings()->getTextColor(&fgColor[0], &fgColor[1], &fgColor[2]);
        }
        fgColorSet = true;
    }
    QColor fgCol;
    fgCol.setRgbF(Image::clamp(fgColor[0], 0., 1.), Image::clamp(fgColor[1], 0., 1.), Image::clamp(fgColor[2], 0., 1.));

    setStyleSheet(QString::fromUtf8("QLabel {\n"
                                    "color: rgb(%1, %2, %3);\n"
                                    "%4\n"
                                    "}\n").arg(fgCol.red()).arg(fgCol.green()).arg(fgCol.blue())
                                          .arg(isBold ? QString::fromUtf8("font-weight: bold;") : QString()));
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
Label::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::EnabledChange) {
        refreshStylesheet();
    }
    QLabel::changeEvent(e);
}


void
Label::setIsModified(bool a)
{
    if ( _customColorSet || isBold ) {
        return;
    }
    StyledKnobWidgetBase::setIsModified(a);
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_Label.cpp"

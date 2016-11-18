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

#include "Label.h"

#include <stdexcept>

#include <QApplication>
#include <QStyle>
#include <QPalette>
#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Gui/GuiApplicationManager.h"


NATRON_NAMESPACE_ENTER;

static QColor rgbToQColor(double r, double g, double b)
{
    QColor c;
    c.setRgbF(Image::clamp(r, 0., 1.),
               Image::clamp(g, 0., 1.),
               Image::clamp(b, 0., 1.));
    return c;
}

Label::Label(const QString &text,
             QWidget *parent,
             Qt::WindowFlags f)
    : QLabel(text, parent, f)
    , textColor()

{
    double r,g,b;
    appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
    setTextColor(rgbToQColor(r, g, b));

    setFont( QApplication::font() ); // necessary, or the labels will get the default font size
}

Label::Label(QWidget *parent,
             Qt::WindowFlags f)
    : QLabel(parent, f)
    , textColor()
{
    double r,g,b;
    appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
    setTextColor(rgbToQColor(r, g, b));

    setFont( QApplication::font() ); // necessary, or the labels will get the default font size
}



void
Label::refreshStyle()
{
    setStyleSheet(QString::fromUtf8("QLabel {color: rgb(%1, %2, %3);}\n"
                                    "QLabel:!enabled { color: black; }").arg(textColor.red()).arg(textColor.green()).arg(textColor.blue()));
    style()->unpolish(this);
    style()->polish(this);
   // QPalette pal = QPalette(palette());
   // pal.setColor(QPalette::Foreground, textColor);
   // setPalette(pal);
    update();
}

const QColor&
Label::getTextColor() const
{
    return textColor;
}

void
Label::setTextColor(const QColor& color)
{
    if (color != textColor) {
        textColor = color;
        refreshStyle();
    }
}

void
Label::setAltered(bool a)
{
    if ( !canAlter() ) {
        return;
    }
    double r,g,b;

    if (a) {
        appPTR->getCurrentSettings()->getAltTextColor(&r, &g, &b);
    } else {
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
    }
    setTextColor(rgbToQColor(r, g, b));

}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Label.cpp"

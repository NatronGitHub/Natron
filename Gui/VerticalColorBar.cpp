/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "VerticalColorBar.h"

#include <stdexcept>

#include <QPainter>
#include <QPen>

#define NATRON_VERTICAL_BAR_WIDTH 2

NATRON_NAMESPACE_ENTER

VerticalColorBar::VerticalColorBar(QWidget* parent)
    : QWidget(parent)
    , _color(Qt::black)
{
    setFixedWidth(NATRON_VERTICAL_BAR_WIDTH);
}

void
VerticalColorBar::setColor(const QColor& color)
{
    _color = color;
    update();
}

QSize
VerticalColorBar::sizeHint() const
{
    return QWidget::sizeHint();
    //return QSize(NATRON_VERTICAL_BAR_WIDTH,1000);
}

void
VerticalColorBar::paintEvent(QPaintEvent* /*e*/)
{
    QPainter p(this);
    QPen pen;

    pen.setCapStyle(Qt::FlatCap);
    pen.setWidth(NATRON_VERTICAL_BAR_WIDTH);
    pen.setColor(_color);
    p.setPen(pen);
    p.drawLine( NATRON_VERTICAL_BAR_WIDTH / 2, NATRON_VERTICAL_BAR_WIDTH, NATRON_VERTICAL_BAR_WIDTH / 2, height() - NATRON_VERTICAL_BAR_WIDTH);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_VerticalColorBar.cpp"

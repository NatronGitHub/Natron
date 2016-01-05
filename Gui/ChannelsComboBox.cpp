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

#include "ChannelsComboBox.h"

#include <cassert>

#include "Global/Macros.h"

#include <QDebug>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QPaintEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QPainter>
#include <QPen>

void
ChannelsComboBox::paintEvent(QPaintEvent* event)
{
    ComboBox::paintEvent(event);

    int idx = activeIndex();
    if (idx != 1) {
        QColor color;

        QPainter p(this);
        QPen pen;

        switch (idx) {
            case 0:
                //luminance
                color.setRgbF(0.5, 0.5, 0.5);
                break;
            case 2:
                //r
                color.setRgbF(1., 0, 0);
                break;
            case 3:
                //g
                color.setRgbF(0., 1., 0.);
                break;
            case 4:
                //b
                color.setRgbF(0., 0. , 1.);
                break;
            case 5:
                //a
                color.setRgbF(1.,1.,1.);
                break;
        }

        pen.setColor(color);
        p.setPen(pen);


        QRectF bRect = rect();
        QRectF roundedRect = bRect.adjusted(1., 1., -2., -2.);

        double roundPixels = 3;


        QPainterPath path;
        path.addRoundedRect(roundedRect, roundPixels, roundPixels);
        p.drawPath(path);
    }
}

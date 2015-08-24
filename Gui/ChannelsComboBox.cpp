//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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

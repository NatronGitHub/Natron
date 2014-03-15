//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ClickableLabel.h"
#include <QEvent>

void ClickableLabel::mousePressEvent(QMouseEvent *e) {
    if (isEnabled()) {
        _toggled = !_toggled;
        emit clicked(_toggled);
    }
    QLabel::mousePressEvent(e);
}

void ClickableLabel::changeEvent(QEvent* e) {
    if (e->type() == QEvent::EnabledChange) {
        if (!isEnabled()) {
            QString str = text();
            QString paintTxt = QString("<font color=\"#000000\">%1</font>").arg(str);
            setText(paintTxt);
        } else {
            QString str = text();
            str = str.remove("<font color=\"#000000\">");
            str = str.remove("</font>");
            setText(str);
        }
    }
}

void ClickableLabel::setText_overload(const QString& str) {
    if (!isEnabled()) {
        QString paintTxt = QString("<font color=\"#000000\">%1</font>").arg(str);
        setText(paintTxt);
    } else {
        setText(str);
    }

}
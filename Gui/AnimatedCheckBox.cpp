//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AnimatedCheckBox.h"

#include <QStyle>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)

void AnimatedCheckBox::setAnimation(int i)
{
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
}

void AnimatedCheckBox::setReadOnly(bool readOnly)
{
    this->readOnly = readOnly;
    style()->unpolish(this);
    style()->polish(this);
}

void AnimatedCheckBox::mousePressEvent(QMouseEvent* e)
{
    if (readOnly && e->buttons().testFlag(Qt::LeftButton)) {
        return;
    } else {
        QCheckBox::mousePressEvent(e);
    }
}
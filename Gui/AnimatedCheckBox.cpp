//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "AnimatedCheckBox.h"

#include <QStyle>
#include "Gui/GuiMacros.h"
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)
CLANG_DIAG_ON(deprecated-register)

void
AnimatedCheckBox::setAnimation(int i)
{
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
}

void
AnimatedCheckBox::setReadOnly(bool readOnly)
{
    this->readOnly = readOnly;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void
AnimatedCheckBox::setDirty(bool b)
{
    dirty = b;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void
AnimatedCheckBox::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space) {
        setChecked(!isChecked());
    } else {
        QCheckBox::keyPressEvent(e);
    }
    
}

void
AnimatedCheckBox::mousePressEvent(QMouseEvent* e)
{
    if ( readOnly && buttonDownIsLeft(e) ) {
        return;
    } else {
        QCheckBox::mousePressEvent(e);
    }
}


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

#include "ClickableLabel.h"

#include <stdexcept>

#include <QApplication>
#include <QEvent>
#include <QStyle>

#include "Gui/Label.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/GuiApplicationManager.h"

NATRON_NAMESPACE_ENTER;

ClickableLabel::ClickableLabel(const QString &text,
                               QWidget *parent)
    : Label(text, parent),
      _toggled(false),
      dirty(false),
      readOnly(false),
      animation(0),
      sunkenStyle(false)
{
}

ClickableLabel::ClickableLabel(const QPixmap &icon,
               QWidget *parent)
: Label(parent),
_toggled(false),
dirty(false),
readOnly(false),
animation(0),
sunkenStyle(false)
{
    setPixmap(icon);
}

void
ClickableLabel::mousePressEvent(QMouseEvent* e)
{
    if ( isEnabled() ) {
        _toggled = !_toggled;
        Q_EMIT clicked(_toggled);
    }
    Label::mousePressEvent(e);
}

void
ClickableLabel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::EnabledChange) {
        if ( !isEnabled() ) {
            QString paintTxt = text();
            paintTxt.prepend("<font color=\"#000000\">");
            paintTxt.append("</font>");
            setText(paintTxt);
        } else {
            QString str = text();
            str = str.remove("<font color=\"#000000\">");
            str = str.remove("</font>");
            setText(str);
        }
    }
}

void
ClickableLabel::setText_overload(const QString & str)
{
    QString paintTxt = str;

    if ( !isEnabled() ) {
        paintTxt.prepend("<font color=\"#000000\">");
        paintTxt.append("</font>");
    }
    setText(paintTxt);
}

void
ClickableLabel::setReadOnly(bool readOnly)
{
    if (this->readOnly != readOnly) {
        this->readOnly = readOnly;
        refreshStyle();
    }
}

void
ClickableLabel::setAnimation(int i)
{
    if (this->animation != i) {
        animation = i;
        refreshStyle();
    }
}

void
ClickableLabel::setDirty(bool b)
{
    if (this->dirty != b) {
        dirty = b;
        refreshStyle();
    }
}

void
ClickableLabel::setSunken(bool s)
{
    if (this->sunkenStyle != s) {
        sunkenStyle = s;
        refreshStyle();
    }
}

KnobClickableLabel::KnobClickableLabel(const QPixmap& icon, KnobGui* knob, QWidget* parent )
: ClickableLabel(icon, parent)
, KnobWidgetDnD(knob,-1, this)
{
    knob->enableRightClickMenu(this, -1);
}

KnobClickableLabel::KnobClickableLabel(const QString& text, KnobGui* knob, QWidget* parent)
: ClickableLabel(text, parent)
, KnobWidgetDnD(knob, -1, this)
{
    knob->enableRightClickMenu(this, -1);
}

void
KnobClickableLabel::enterEvent(QEvent* e)
{
    mouseEnterDnD(e);
    ClickableLabel::enterEvent(e);
}

void
KnobClickableLabel::leaveEvent(QEvent* e)
{
    mouseLeaveDnD(e);
    ClickableLabel::leaveEvent(e);
}

void
KnobClickableLabel::keyPressEvent(QKeyEvent* e)
{
    keyPressDnD(e);
    ClickableLabel::keyPressEvent(e);
}

void
KnobClickableLabel::keyReleaseEvent(QKeyEvent* e)
{
    keyReleaseDnD(e);
    ClickableLabel::keyReleaseEvent(e);
}


void
KnobClickableLabel::mousePressEvent(QMouseEvent* e)
{
    if (!mousePressDnD(e)) {
        ClickableLabel::mousePressEvent(e);
    }
}

void
KnobClickableLabel::mouseMoveEvent(QMouseEvent* e)
{
    if (!mouseMoveDnD(e)) {
        ClickableLabel::mouseMoveEvent(e);
    }
}

void
KnobClickableLabel::mouseReleaseEvent(QMouseEvent* e)
{
    mouseReleaseDnD(e);
    ClickableLabel::mouseReleaseEvent(e);
    
}

void
KnobClickableLabel::dragEnterEvent(QDragEnterEvent* e)
{
    if (!dragEnterDnD(e)) {
        ClickableLabel::dragEnterEvent(e);
    }
}

void
KnobClickableLabel::dragMoveEvent(QDragMoveEvent* e)
{
    if (!dragMoveDnD(e)) {
        ClickableLabel::dragMoveEvent(e);
    }
}
void
KnobClickableLabel::dropEvent(QDropEvent* e)
{
    if (!dropDnD(e)) {
        ClickableLabel::dropEvent(e);
    }
}

void
KnobClickableLabel::focusInEvent(QFocusEvent* e)
{
    focusInDnD();
    ClickableLabel::focusInEvent(e);
}

void
KnobClickableLabel::focusOutEvent(QFocusEvent* e)
{
    focusOutDnD();
    ClickableLabel::focusOutEvent(e);
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ClickableLabel.cpp"

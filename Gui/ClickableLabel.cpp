/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <QMouseEvent>
#include <QStyle>

#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiWidgets.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobWidgetDnD.h"

NATRON_NAMESPACE_ENTER

ClickableLabel::ClickableLabel(const QString &text,
                               QWidget *parent)
    : Label(text, parent)
    , _toggled(false)
{
}

ClickableLabel::ClickableLabel(const QPixmap &icon,
                               QWidget *parent)
    : Label(parent)
    , _toggled(false)
{
    setPixmap(icon);
}

void
ClickableLabel::mousePressEvent(QMouseEvent* e)
{
    if ( isEnabled() && buttonDownIsLeft(e)) {
        _toggled = !_toggled;
        Q_EMIT clicked(_toggled);
    }
    Label::mousePressEvent(e);
}


KnobClickableLabel::KnobClickableLabel(const QPixmap& icon,
                                       const KnobGuiPtr& knob,
                                       ViewSetSpec view,
                                       QWidget* parent )
    : ClickableLabel(icon, parent)
, _dnd(KnobWidgetDnD::create(knob, DimSpec::all(), view, this) )
{
    KnobGuiWidgets::enableRightClickMenu(knob, this, DimSpec::all(), view);
}

KnobClickableLabel::KnobClickableLabel(const QString& text,
                                       const KnobGuiPtr& knob,
                                       ViewSetSpec view,
                                       QWidget* parent)
    : ClickableLabel(text, parent)
, _dnd( KnobWidgetDnD::create(knob, DimSpec::all(), view,this) )
{
    KnobGuiWidgets::enableRightClickMenu(knob, this, DimSpec::all(), view);
}

KnobClickableLabel::~KnobClickableLabel()
{
}

void
KnobClickableLabel::enterEvent(QEvent* e)
{
    _dnd->mouseEnter(e);
    ClickableLabel::enterEvent(e);
}

void
KnobClickableLabel::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    ClickableLabel::leaveEvent(e);
}

void
KnobClickableLabel::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    ClickableLabel::keyPressEvent(e);
}

void
KnobClickableLabel::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    ClickableLabel::keyReleaseEvent(e);
}

void
KnobClickableLabel::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        ClickableLabel::mousePressEvent(e);
    }
}

void
KnobClickableLabel::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        ClickableLabel::mouseMoveEvent(e);
    }
}

void
KnobClickableLabel::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    ClickableLabel::mouseReleaseEvent(e);
}

void
KnobClickableLabel::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        ClickableLabel::dragEnterEvent(e);
    }
}

void
KnobClickableLabel::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        ClickableLabel::dragMoveEvent(e);
    }
}

void
KnobClickableLabel::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        ClickableLabel::dropEvent(e);
    }
}

void
KnobClickableLabel::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    ClickableLabel::focusInEvent(e);
}

void
KnobClickableLabel::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    ClickableLabel::focusOutEvent(e);
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_ClickableLabel.cpp"


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

#include "ClickableLabel.h"

#include <stdexcept>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QStyle>

#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"
#include "Gui/KnobGui.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobWidgetDnD.h"

NATRON_NAMESPACE_ENTER

ClickableLabel::ClickableLabel(const QString &text,
                               QWidget *parent)
    : Label(text, parent)
    , _toggled(false)
    , _bold(false)
    , _dirty(false)
    , _readOnly(false)
    , _animation(0)
    , _sunkenStyle(false)
{
}

ClickableLabel::ClickableLabel(const QPixmap &icon,
                               QWidget *parent)
    : Label(parent)
    , _toggled(false)
    , _bold(false)
    , _dirty(false)
    , _readOnly(false)
    , _animation(0)
    , _sunkenStyle(false)
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

void
ClickableLabel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::EnabledChange) {
        if ( !isEnabled() ) {
            QString paintTxt = text();
            paintTxt.prepend( QString::fromUtf8("<font color=\"#000000\">") );
            paintTxt.append( QString::fromUtf8("</font>") );
            setText(paintTxt);
        } else {
            QString str = text();
            str = str.remove( QString::fromUtf8("<font color=\"#000000\">") );
            str = str.remove( QString::fromUtf8("</font>") );
            setText(str);
        }
    }
}

void
ClickableLabel::setText_overload(const QString & str)
{
    QString paintTxt = str;

    if ( !isEnabled() ) {
        paintTxt.prepend( QString::fromUtf8("<font color=\"#000000\">") );
        paintTxt.append( QString::fromUtf8("</font>") );
    }
    if (_bold) {
        paintTxt.prepend( QString::fromUtf8("<b>") );
        paintTxt.append( QString::fromUtf8("</b>") );
    }
    setText(paintTxt);
}

void
ClickableLabel::setBold(bool b)
{
    _bold = b;
}

bool
ClickableLabel::canAlter() const
{
    return !_bold;
}

void
ClickableLabel::setReadOnly(bool readOnly)
{
    if (_readOnly != readOnly) {
        _readOnly = readOnly;
        refreshStyle();
    }
}

void
ClickableLabel::setAnimation(int i)
{
    if (_animation != i) {
        _animation = i;
        refreshStyle();
    }
}

void
ClickableLabel::setDirty(bool b)
{
    if (_dirty != b) {
        _dirty = b;
        refreshStyle();
    }
}

void
ClickableLabel::setSunken(bool s)
{
    if (_sunkenStyle != s) {
        _sunkenStyle = s;
        refreshStyle();
    }
}

KnobClickableLabel::KnobClickableLabel(const QPixmap& icon,
                                       const KnobGuiPtr& knob,
                                       QWidget* parent )
    : ClickableLabel(icon, parent)
, _dnd(KnobWidgetDnD::create(knob, -1, this) )
{
    knob->enableRightClickMenu(this, -1);
}

KnobClickableLabel::KnobClickableLabel(const QString& text,
                                       const KnobGuiPtr& knob,
                                       QWidget* parent)
    : ClickableLabel(text, parent)
    , _dnd( KnobWidgetDnD::create(knob, -1, this) )
{
    knob->enableRightClickMenu(this, -1);
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

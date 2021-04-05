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

#include "ViewerToolButton.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QStyle>
#include <QMouseEvent>
#include <QtCore/QTimer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiMacros.h"


NATRON_NAMESPACE_ENTER


ViewerToolButton::ViewerToolButton(QWidget* parent)
    : QToolButton(parent)
    , isSelected(false)
    , wasMouseReleased(false)
{
    setFocusPolicy(Qt::ClickFocus);
}

ViewerToolButton::~ViewerToolButton()
{
}

void
ViewerToolButton::mousePressEvent(QMouseEvent* /*e*/)
{
    setFocus();
    wasMouseReleased = false;
    QTimer::singleShot( 300, this, SLOT(handleLongPress()) );
}

void
ViewerToolButton::handleLongPress()
{
    if (!wasMouseReleased) {
        showMenu();
    }
}

void
ViewerToolButton::mouseReleaseEvent(QMouseEvent* e)
{
    wasMouseReleased = true;
    if ( triggerButtonIsRight(e) ) {
        showMenu();
    } else if ( triggerButtonIsLeft(e) ) {
        handleSelection();
    } else {
        QToolButton::mousePressEvent(e);
    }
}

bool
ViewerToolButton::getIsSelected() const
{
    return isSelected;
}

void
ViewerToolButton::setIsSelected(bool s)
{
    if (s != isSelected) {
        isSelected = s;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
ViewerToolButton::handleSelection()
{
    QAction* curAction = defaultAction();

    if (!curAction) {
        return;
    }
    if ( !isDown() ) {
        setDown(true);
        Q_EMIT triggered(curAction);
    } else {
        QList<QAction*> allAction = actions();
        for (int i = 0; i < allAction.size(); ++i) {
            if (allAction[i] == curAction) {
                int next = ( i == (allAction.size() - 1) ) ? 0 : i + 1;
                setDefaultAction(allAction[next]);
                Q_EMIT triggered(allAction[next]);
                break;
            }
        }
    }
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_ViewerToolButton.cpp"

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "PanelWidget.h"

#include <cassert>
#include <stdexcept>

#include <QUndoStack>
#include <QApplication>
#include <QKeyEvent>
#include "Engine/UndoCommand.h"
#include "Gui/TabWidget.h"
#include "Gui/Gui.h"
#include "Gui/UndoCommand_qt.h"

NATRON_NAMESPACE_ENTER

PanelWidget::PanelWidget(const std::string& scriptName,
                         QWidget* thisWidget,
                         Gui* gui)
    : ScriptObject()
    , _thisWidget(thisWidget)
    , _gui(gui)
{
    assert(_gui && _thisWidget);
    setScriptNameInternal(scriptName, false);
    _gui->registerTab(this, this);
}

PanelWidget::~PanelWidget()
{
}

Gui*
PanelWidget::getGui() const
{
    return _gui;
}

void
PanelWidget::notifyGuiClosingPublic()
{
    _gui = 0;
    notifyGuiClosing();
}

static TabWidget*
getParentPaneRecursive(QWidget* w)
{
    if (!w) {
        return 0;
    }
    TabWidget* tw = dynamic_cast<TabWidget*>(w);
    if (tw) {
        return tw;
    } else {
        return getParentPaneRecursive( w->parentWidget() );
    }
}

TabWidget*
PanelWidget::getParentPane() const
{
    return getParentPaneRecursive(_thisWidget);
}

bool
PanelWidget::enterEventBase()
{
    TabWidget* parentPane = getParentPane();

    if (parentPane) {
        parentPane->setWidgetMouseOverFocus(this, true);
    }
    if ( _gui && _gui->isFocusStealingPossible() ) {
        _thisWidget->setFocus();

        //Make this stack the active one
        boost::shared_ptr<QUndoStack> stack = getUndoStack();
        if (stack) {
            stack->setActive();
        }
        return true;
    }
    return false;
}

void
PanelWidget::pushUndoCommand(QUndoCommand* command)
{
    boost::shared_ptr<QUndoStack> stack = getUndoStack();

    if (stack) {
        stack->setActive();
        stack->push(command);
    } else {
        //You are trying to push a command to a widget that do not own a stack!
        assert(false);
    }
}

void
PanelWidget::pushUndoCommand(const UndoCommandPtr& command)
{
    pushUndoCommand(new UndoCommand_qt(command));
}

void
PanelWidget::pushUndoCommand(UndoCommand* command)
{
    UndoCommandPtr p(command);
    pushUndoCommand(p);
}

bool
PanelWidget::isClickFocusPanel() const
{
    if (!_gui) {
        return false;
    }

    return _gui->getCurrentPanelFocus() == this;
}

void
PanelWidget::takeClickFocus()
{
    TabWidget* parentPane = getParentPane();

    if (parentPane) {
        parentPane->setWidgetClickFocus(this, true);
    }
    if (_gui) {
        const RegisteredTabs& tabs = _gui->getRegisteredTabs();
        for (RegisteredTabs::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
            if (it->second.first != this) {
                it->second.first->removeClickFocus();
            }
        }
        _gui->setCurrentPanelFocus(this);
    }
}

void
PanelWidget::removeClickFocus()
{
    TabWidget* parentPane = getParentPane();

    if (parentPane) {
        parentPane->setWidgetClickFocus(this, false);
    }
    if (_gui) {
        _gui->setCurrentPanelFocus(0);
    }
}

void
PanelWidget::leaveEventBase()
{
    TabWidget* parentPane = getParentPane();

    if (parentPane) {
        parentPane->setWidgetMouseOverFocus(this, false);
    }
}

void
PanelWidget::handleUnCaughtKeyUpEvent(QKeyEvent* e)
{
    if (!_gui) {
        return;
    }
    _gui->setLastKeyUpVisitedClickFocus(_gui->getCurrentPanelFocus() == this);
    TabWidget* parentPane = getParentPane();
    if ( parentPane && parentPane->isFloatingWindowChild() ) {
        //We have to send the event to the Gui object, because it won't receive it as they are part from different windows
        qApp->sendEvent(_gui, e);
    }
}

void
PanelWidget::handleUnCaughtKeyPressEvent(QKeyEvent* e)
{
    if (!_gui) {
        return;
    }
    _gui->setLastKeyPressVisitedClickFocus(_gui->getCurrentPanelFocus() == this);
    TabWidget* parentPane = getParentPane();
    if ( parentPane && parentPane->isFloatingWindowChild() ) {
        //We have to send the event to the Gui object, because it won't receive it as they are part from different windows
        qApp->sendEvent(_gui, e);
    }
}


void
PanelWidget::onScriptNameChanged()
{
    getGui()->unregisterTab(this);
    getGui()->registerTab(this, this);
}

void
PanelWidget::onLabelChanged()
{

}


NATRON_NAMESPACE_EXIT

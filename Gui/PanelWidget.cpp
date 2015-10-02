/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Gui/TabWidget.h"
#include "Gui/Gui.h"

PanelWidget::PanelWidget(QWidget* thisWidget,
                         Gui* gui)
: ScriptObject()
, _thisWidget(thisWidget)
, _gui(gui)
{
    assert(_gui && _thisWidget);
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

static TabWidget* getParentPaneRecursive(QWidget* w)
{
    if (!w) {
        return 0;
    }
    TabWidget* tw = dynamic_cast<TabWidget*>(w);
    if (tw) {
        return tw;
    } else {
        return getParentPaneRecursive(w->parentWidget());
    }
}

TabWidget*
PanelWidget::getParentPane() const
{
    return getParentPaneRecursive(_thisWidget);
}

void
PanelWidget::enterEventBase()
{
    
    TabWidget* parentPane = getParentPane();
    if (parentPane) {
        parentPane->setWidgetMouseOverFocus(this, true);
    }
    if (_gui && _gui->isFocusStealingPossible()) {
        _thisWidget->setFocus();
    }

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
        for (RegisteredTabs::const_iterator it = tabs.begin(); it!=tabs.end(); ++it) {
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
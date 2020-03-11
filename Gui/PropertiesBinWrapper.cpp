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

#include "PropertiesBinWrapper.h"

#include <stdexcept>

#include <QApplication>
#include <QMouseEvent>

#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/TabWidget.h"

NATRON_NAMESPACE_ENTER

PropertiesBinWrapper::PropertiesBinWrapper(const std::string& scriptName, Gui* parent)
    : QWidget(parent)
    , PanelWidget(scriptName, this, parent)
{
    setMouseTracking(true);
}

PropertiesBinWrapper::~PropertiesBinWrapper()
{
}

void
PropertiesBinWrapper::mousePressEvent(QMouseEvent* e)
{
    takeClickFocus();
    QWidget::mousePressEvent(e);
}

void
PropertiesBinWrapper::mouseMoveEvent(QMouseEvent* e)
{
    TabWidget* tab = getParentPane() ;
    if (tab) {
        // If the Viewer is in a tab, send the tab widget the event directly
        qApp->sendEvent(tab, e);
    } else {
        QWidget::mouseMoveEvent(e);
    }

    
}

void
PropertiesBinWrapper::enterEvent(QEvent* e)
{
    QWidget::enterEvent(e);
}

void
PropertiesBinWrapper::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
PropertiesBinWrapper::keyPressEvent(QKeyEvent* e)
{
    handleUnCaughtKeyPressEvent(e);
    QWidget::keyPressEvent(e);
}

void
PropertiesBinWrapper::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QWidget::keyReleaseEvent(e);
}

boost::shared_ptr<QUndoStack>
PropertiesBinWrapper::getUndoStack() const
{
    QWidget* w = qApp->widgetAt( QCursor::pos() );
    RightClickableWidget* panel = RightClickableWidget::isParentSettingsPanelRecursive(w);

    if (panel) {
        QUndoStackPtr stack = panel->getPanel()->getUndoStack();

        return stack;
    }

    return boost::shared_ptr<QUndoStack>();
}


QIcon
PropertiesBinWrapper::getIcon() const
{
    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap p;
    appPTR->getIcon(NATRON_PIXMAP_PROPERTIES_PANEL, iconSize, &p);
    return QIcon(p);
}


NATRON_NAMESPACE_EXIT

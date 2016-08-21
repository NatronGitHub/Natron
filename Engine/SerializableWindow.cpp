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

#include "SerializableWindow.h"

#include <stdexcept>

#include <QtCore/QMutex>

#include "Engine/TabWidgetI.h"
#include "Engine/SplitterI.h"
#include "Engine/DockablePanelI.h"
#include "Engine/ProjectSerialization.h"

NATRON_NAMESPACE_ENTER;

SerializableWindow::SerializableWindow()
    : _lock(new QMutex)
    , _w(0), _h(0), _x(0), _y(0)
{
}

SerializableWindow::~SerializableWindow()
{
    delete _lock;
}

void
SerializableWindow::setMtSafeWindowSize(int w,
                                        int h)
{
    QMutexLocker k(_lock);

    _w = w;
    _h = h;
}

void
SerializableWindow::setMtSafePosition(int x,
                                      int y)
{
    QMutexLocker k(_lock);

    _x = x;
    _y = y;
}

void
SerializableWindow::getMtSafeWindowSize(int &w,
                                        int & h)
{
    QMutexLocker k(_lock);

    w = _w;
    h = _h;
}

void
SerializableWindow::getMtSafePosition(int &x,
                                      int &y)
{
    QMutexLocker k(_lock);

    x = _x;
    y = _y;
}

/**
 * @brief Implement to save the content of the object to the serialization object
 **/
void
SerializableWindow::toSerialization(SerializationObjectBase* serializationBase)
{
    ProjectWindowSerialization* serialization = dynamic_cast<ProjectWindowSerialization*>(serializationBase);
    if (!serialization) {
        return;
    }
    getMtSafePosition(serialization->windowPosition[0], serialization->windowPosition[1]);
    getMtSafeWindowSize(serialization->windowSize[0], serialization->windowSize[1]);
    TabWidgetI* isTab = isMainWidgetTab();
    SplitterI* isSplitter = isMainWidgetSplitter();
    DockablePanelI* isPanel = isMainWidgetPanel();
    assert(isTab || isSplitter || isPanel);
    if (isTab) {
        serialization->childType = eProjectWorkspaceWidgetTypeTabWidget;
        serialization->isChildTabWidget.reset(new ProjectTabWidgetSerialization);
        isTab->toSerialization(serialization->isChildTabWidget.get());
    } else if (isSplitter) {
        serialization->childType = eProjectWorkspaceWidgetTypeSplitter;
        serialization->isChildSplitter.reset(new ProjectWindowSplitterSerialization);
        isSplitter->toSerialization(serialization->isChildSplitter.get());
    } else if (isPanel) {
        serialization->childType = eProjectWorkspaceWidgetTypeSettingsPanel;
        serialization->isChildSettingsPanel = isPanel->getHolderFullyQualifiedScriptName();
    }
}

/**
 * @brief Implement to load the content of the serialization object onto this object
 **/
void
SerializableWindow::fromSerialization(const SerializationObjectBase& serializationBase)
{
    const ProjectWindowSerialization* serialization = dynamic_cast<const ProjectWindowSerialization*>(&serializationBase);
    if (!serialization) {
        return;
    }
    setMtSafePosition(serialization->windowPosition[0], serialization->windowPosition[1]);
    setMtSafeWindowSize(serialization->windowSize[0], serialization->windowSize[1]);
    restoreChildFromSerialization(*serialization);
}

NATRON_NAMESPACE_EXIT;


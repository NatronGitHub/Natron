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

#include "SplitterI.h"
#include "Engine/TabWidgetI.h"
#include "Engine/ProjectSerialization.h"

NATRON_NAMESPACE_ENTER;

void
SplitterI::toSerialization(SerializationObjectBase* serializationBase)
{
    ProjectWindowSplitterSerialization* serialization = dynamic_cast<ProjectWindowSplitterSerialization*>(serializationBase);
    if (!serialization) {
        return;
    }
    serialization->orientation = getNatronOrientation();
    serialization->leftChildSize = getLeftChildrenSize();
    serialization->rightChildSize = getRightChildrenSize();
    {
        serialization->leftChild.reset(new ProjectWindowSplitterSerialization::Child);
        SplitterI* isLeftSplitter = isLeftChildSplitter();
        TabWidgetI* isLeftTabWidget = isLeftChildTabWidget();
        if (isLeftSplitter) {
            serialization->leftChild->type = eProjectWorkspaceWidgetTypeSplitter;
            serialization->leftChild->childIsSplitter.reset(new ProjectWindowSplitterSerialization);
            isLeftSplitter->toSerialization(serialization->leftChild->childIsSplitter.get());
        } else if (isLeftTabWidget) {
            serialization->leftChild->type = eProjectWorkspaceWidgetTypeTabWidget;
            serialization->leftChild->childIsTabWidget.reset(new ProjectTabWidgetSerialization);
            isLeftTabWidget->toSerialization(serialization->leftChild->childIsTabWidget.get());
        }
    }
    {
        serialization->rightChild.reset(new ProjectWindowSplitterSerialization::Child);
        SplitterI* isRightSplitter = isRightChildSplitter();
        TabWidgetI* isRigthTabWidget = isRightChildTabWidget();
        if (isRightSplitter) {
            serialization->rightChild->type = eProjectWorkspaceWidgetTypeSplitter;
            serialization->rightChild->childIsSplitter.reset(new ProjectWindowSplitterSerialization);
            isRightSplitter->toSerialization(serialization->rightChild->childIsSplitter.get());
        } else if (isRigthTabWidget) {
            serialization->rightChild->type = eProjectWorkspaceWidgetTypeTabWidget;
            serialization->rightChild->childIsTabWidget.reset(new ProjectTabWidgetSerialization);
            isRigthTabWidget->toSerialization(serialization->rightChild->childIsTabWidget.get());
        }

    }
}

void
SplitterI::fromSerialization(const SerializationObjectBase& serializationBase)
{
    const ProjectWindowSplitterSerialization* serialization = dynamic_cast<const ProjectWindowSplitterSerialization*>(&serializationBase);
    if (!serialization) {
        return;
    }
    restoreChildrenFromSerialization(*serialization);
    setChildrenSize(serialization->leftChildSize, serialization->rightChildSize);
}

NATRON_NAMESPACE_EXIT;
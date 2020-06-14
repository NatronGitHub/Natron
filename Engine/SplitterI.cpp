/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "SplitterI.h"
#include "Engine/TabWidgetI.h"
#include "Serialization/WorkspaceSerialization.h"

NATRON_NAMESPACE_ENTER

void
SplitterI::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{
    SERIALIZATION_NAMESPACE::WidgetSplitterSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::WidgetSplitterSerialization*>(serializationBase);
    if (!serialization) {
        return;
    }
    OrientationEnum ori = getNatronOrientation();
    switch (ori) {
        case eOrientationHorizontal:
            serialization->orientation = kSplitterOrientationHorizontal;
            break;
        case eOrientationVertical:
            serialization->orientation = kSplitterOrientationVertical;
            break;
    }
    serialization->leftChildSize = getLeftChildrenSize();
    serialization->rightChildSize = getRightChildrenSize();
    {
        SplitterI* isLeftSplitter = isLeftChildSplitter();
        TabWidgetI* isLeftTabWidget = isLeftChildTabWidget();
        if (isLeftSplitter) {
            serialization->leftChild.type = kSplitterChildTypeSplitter;
            serialization->leftChild.childIsSplitter.reset(new SERIALIZATION_NAMESPACE::WidgetSplitterSerialization);
            isLeftSplitter->toSerialization(serialization->leftChild.childIsSplitter.get());
        } else if (isLeftTabWidget) {
            serialization->leftChild.type = kSplitterChildTypeTabWidget;
            serialization->leftChild.childIsTabWidget.reset(new SERIALIZATION_NAMESPACE::TabWidgetSerialization);
            isLeftTabWidget->toSerialization(serialization->leftChild.childIsTabWidget.get());
        }
    }
    {
        SplitterI* isRightSplitter = isRightChildSplitter();
        TabWidgetI* isRigthTabWidget = isRightChildTabWidget();
        if (isRightSplitter) {
            serialization->rightChild.type = kSplitterChildTypeSplitter;
            serialization->rightChild.childIsSplitter.reset(new SERIALIZATION_NAMESPACE::WidgetSplitterSerialization);
            isRightSplitter->toSerialization(serialization->rightChild.childIsSplitter.get());
        } else if (isRigthTabWidget) {
            serialization->rightChild.type = kSplitterChildTypeTabWidget;
            serialization->rightChild.childIsTabWidget.reset(new SERIALIZATION_NAMESPACE::TabWidgetSerialization);
            isRigthTabWidget->toSerialization(serialization->rightChild.childIsTabWidget.get());
        }

    }
}

void
SplitterI::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{
    const SERIALIZATION_NAMESPACE::WidgetSplitterSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::WidgetSplitterSerialization*>(&serializationBase);
    if (!serialization) {
        return;
    }
    restoreChildrenFromSerialization(*serialization);
    setChildrenSize(serialization->leftChildSize, serialization->rightChildSize);
}

NATRON_NAMESPACE_EXIT

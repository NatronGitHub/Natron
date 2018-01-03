/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef SERIALIZABLEWINDOW_H
#define SERIALIZABLEWINDOW_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A serializable window is a window which have mutex-protected members that can be accessed in the serialization thread
 * because width() height() pos() etc... are not thread-safe.
 **/
class SerializableWindow : public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
    mutable QMutex* _lock;
    int _w, _h;
    int _x, _y;

public:

    SerializableWindow();

    virtual ~SerializableWindow();

    /**
     * @brief Should be called in resizeEvent
     **/
    void setMtSafeWindowSize(int w, int h);

    /**
     * @brief Should be called in moveEvent
     **/
    void setMtSafePosition(int x, int y);

    void getMtSafeWindowSize(int &w, int & h);

    void getMtSafePosition(int &x, int &y);

    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

    virtual TabWidgetI* isMainWidgetTab() const = 0;

    virtual SplitterI* isMainWidgetSplitter() const = 0;

    virtual DockablePanelI* isMainWidgetPanel() const = 0;

protected:

    virtual void restoreChildFromSerialization(const SERIALIZATION_NAMESPACE::WindowSerialization& serialization) = 0;
    
};

NATRON_NAMESPACE_EXIT


#endif // SERIALIZABLEWINDOW_H

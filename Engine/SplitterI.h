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

#ifndef SPLITTERI_H
#define SPLITTERI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Serialization/SerializationBase.h"
#include "Global/Enums.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Interface for the Splitter class
 **/
class SplitterI : public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
public:

    SplitterI()
    {

    }

    virtual ~SplitterI()
    {

    }

    virtual OrientationEnum getNatronOrientation() const = 0;

    virtual void setNatronOrientation(OrientationEnum orientation) = 0;

    virtual void setChildrenSize(int left, int right) = 0;

    virtual int getLeftChildrenSize() const = 0;

    virtual int getRightChildrenSize() const = 0;

    virtual SplitterI* isLeftChildSplitter() const = 0;

    virtual TabWidgetI* isLeftChildTabWidget() const = 0;

    virtual SplitterI* isRightChildSplitter() const = 0;

    virtual TabWidgetI* isRightChildTabWidget() const = 0;


    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

protected:

    virtual void restoreChildrenFromSerialization(const SERIALIZATION_NAMESPACE::WidgetSplitterSerialization& serialization) = 0;
};

NATRON_NAMESPACE_EXIT

#endif // SPLITTERI_H

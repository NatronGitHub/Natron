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

#ifndef PYPANELI_H
#define PYPANELI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class PyPanelI : public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
public:

    PyPanelI()
    {

    }

    virtual ~PyPanelI()
    {

    }

    virtual void setPythonFunction(const QString& function) = 0;

    virtual QString getPythonFunction() const  = 0;

    virtual void setPanelLabel(const QString& label) = 0;

    virtual QString getPanelLabel() const = 0;

    virtual QString getPanelScriptName() const = 0;

    virtual KnobsVec getKnobs() const = 0;

    virtual QString save_serialization_thread() const = 0;

    virtual void restore(const QString& data) = 0;

    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

};

NATRON_NAMESPACE_EXIT

#endif // PYPANELI_H

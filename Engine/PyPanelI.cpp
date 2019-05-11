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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyPanelI.h"
#include "Engine/KnobTypes.h"

#include "Serialization/KnobSerialization.h"
#include "Serialization/WorkspaceSerialization.h"

SERIALIZATION_NAMESPACE_USING

NATRON_NAMESPACE_ENTER

void
PyPanelI::toSerialization(SerializationObjectBase* serializationBase)
{
    PythonPanelSerialization* serialization = dynamic_cast<PythonPanelSerialization*>(serializationBase);
    if (!serialization) {
        return;
    }
    serialization->name = getPanelLabel().toStdString();
    KnobsVec knobs = getKnobs();
    for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobGroupPtr isGroup = toKnobGroup(*it);
        KnobPagePtr isPage = toKnobPage(*it);

        if (!isGroup && !isPage) {
            KnobSerializationPtr k = boost::make_shared<KnobSerialization>();
            (*it)->toSerialization(k.get());
            if (k->_mustSerialize) {
                serialization->knobs.push_back(k);
            }
        }
    }

    serialization->pythonFunction = getPythonFunction().toStdString();
    serialization->userData = save_serialization_thread().toStdString();
}

void
PyPanelI::fromSerialization(const SerializationObjectBase& serializationBase)
{
    const PythonPanelSerialization* serialization = dynamic_cast<const PythonPanelSerialization*>(&serializationBase);
    if (!serialization) {
        return;
    }

    setPythonFunction(QString::fromUtf8(serialization->pythonFunction.c_str()));
    restore(QString::fromUtf8(serialization->userData.c_str()));
    setPanelLabel(QString::fromUtf8(serialization->name.c_str()));

    KnobsVec knobs = getKnobs();
    for (std::list<KnobSerializationPtr>::const_iterator it = serialization->knobs.begin(); it!=serialization->knobs.end(); ++it) {
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it) {
            if ((*it2)->getName() == (*it)->getName()) {
                (*it2)->fromSerialization(**it);
                break;
            }
        }
    }

}

NATRON_NAMESPACE_EXIT

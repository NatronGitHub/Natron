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

#ifdef PYSIDE_OLD

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cassert>
#include <stdexcept>

// Compatibility function for pyside versions before this commit:
// https://qt.gitorious.org/pyside/pyside/commit/b3669dca4e4321b204d10b06018d35900b1847ee

#include <pyside.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
#include <basewrapper.h>
#include <conversions.h>
#include <sbkconverter.h>
#include <gilstate.h>
#include <typeresolver.h>
#include <bindingmanager.h>
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_ON(missing-field-initializers)

#include <algorithm>
#include <cstring>
#include <cctype>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QStack>
#include <QCoreApplication>
#include <QDebug>
#include <QSharedPointer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/GlobalFunctions.h"

// A QSharedPointer is used with a deletion function to invalidate a pointer
// when the property value is cleared. This should be a QSharedPointer with
// a void* pointer, but that isn't allowed
typedef char any_t;
Q_DECLARE_METATYPE(QSharedPointer<any_t>);

namespace PySide
{

    static void invalidatePtr(any_t* object)
    {
        Shiboken::GilState state;

        SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(object);
        if (wrapper != NULL)
            Shiboken::BindingManager::instance().releaseWrapper(wrapper);
    }

    static const char invalidatePropertyName[] = "_PySideInvalidatePtr";

    PyObject* getWrapperForQObject(QObject* cppSelf, SbkObjectType* sbk_type)
    {
        PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppSelf);
        if (pyOut) {
            Py_INCREF(pyOut);
            return pyOut;
        }

        // Setting the property will trigger an QEvent notification, which may call into
        // code that creates the wrapper so only set the property if it isn't already
        // set and check if it's created after the set call
        QVariant existing = cppSelf->property(invalidatePropertyName);
        if (!existing.isValid()) {
            QSharedPointer<any_t> shared_with_del((any_t*)cppSelf, invalidatePtr);
            cppSelf->setProperty(invalidatePropertyName, QVariant::fromValue(shared_with_del));
            pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppSelf);
            if (pyOut) {
                Py_INCREF(pyOut);
                return pyOut;
            }
        }
        
        const char* typeName = typeid(*cppSelf).name();
        pyOut = Shiboken::Object::newObject(sbk_type, cppSelf, false, false, typeName);
        
        return pyOut;
    }
} //namespace PySide

#endif

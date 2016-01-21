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

#ifndef Natron_Engine_ThreadStorage_h
#define Natron_Engine_ThreadStorage_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtCore/QCoreApplication>

#include "Engine/EngineFwd.h"

// A class that inherits from QThreadStorage, but never sets local data in the main thread.
// It uses an actual instance of the data.
// That way, the ThreadStorage class can be destroyed before leaving the main thread.
//
// Of course, as a consequence the main thread always has "local data", which is just a global variable.

NATRON_NAMESPACE_ENTER;
template <class T>
class ThreadStorage
    : public QThreadStorage<T>
{
public:
    ThreadStorage() : mainData() {}

    /// Is local storage present?
    /// This *does not* mean that it was initialized.
    /// For example, local data on the main thread is
    /// always present by may not be initialized.
    /// Do *not* use this to check if there is *valid* local data. You must store a flag in the local data for that purpose.
    inline bool hasLocalData() const
    {
        return (qApp && QThread::currentThread() == qApp->thread()) || QThreadStorage<T>::hasLocalData();
    }

    inline T & localData()
    {
        if ( qApp && QThread::currentThread() == qApp->thread() ) {
            return mainData;
        } else {
            return QThreadStorage<T>::localData();
        }
    }

    inline T localData() const
    {
        if ( qApp && QThread::currentThread() == qApp->thread() ) {
            return mainData;
        } else {
            return QThreadStorage<T>::localData();
        }
    }

    inline void setLocalData(const T& t)
    {
        if ( qApp && QThread::currentThread() == qApp->thread() ) {
            mainData = t;
        } else {
            return QThreadStorage<T>::setLocalData(t);
        }
    }

private:
    T mainData;
};
NATRON_NAMESPACE_EXIT;


#endif // ifndef Natron_Engine_ThreadStorage_h

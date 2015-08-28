/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_SINGLETON_H_
#define NATRON_ENGINE_SINGLETON_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cstdlib> // for std::atexit()
#include <QtCore/QMutexLocker>

// Singleton pattern ( thread-safe) , to have 1 global ptr

template<class T>
class Singleton
{
protected:
    inline explicit Singleton()
    {
        // _lock = new QMutex;
        Singleton::instance_ = static_cast<T*>(this);
    }

    inline ~Singleton()
    {
        Singleton::instance_ = 0;
    }

public:
    friend class QMutex;
    static T* instance();
    static void Destroy();
    static QMutex* _mutex;

private:
    static T* instance_;
    inline explicit Singleton(const Singleton &);
    inline Singleton & operator=(const Singleton &)
    {
        return *this;
    };
    static T* CreateInstance();
    static void ScheduleForDestruction( void (*)() );
    static void DestroyInstance(T*);
};

template<class T>
T*
Singleton<T>::instance()
{
    if (Singleton::instance_ == 0) {
        if (_mutex) {
            QMutexLocker locker(_mutex);
        }
        if (Singleton::instance_ == 0) {
            Singleton::instance_ = CreateInstance();
            ScheduleForDestruction(Singleton::Destroy);
        }
    }

    return Singleton::instance_;
}

template<class T>
void
Singleton<T>::Destroy()
{
    if (Singleton::instance_ != 0) {
        DestroyInstance(Singleton::instance_);
        Singleton::instance_ = 0;
    }
}

template<class T>
T*
Singleton<T>::CreateInstance()
{
    return new T();
}

template<class T>
void
Singleton<T>::ScheduleForDestruction( void (*pFun)() )
{
    std::atexit(pFun);
}

template<typename T>
void
Singleton<T>::DestroyInstance(T* p)
{
    delete p;
}

template<class T>
T * Singleton<T>::instance_ = 0;
template<class T>
QMutex * Singleton<T>::_mutex = 0;


#endif // ifndef NATRON_ENGINE_SINGLETON_H_

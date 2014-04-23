//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 18/03/2014.
//
//

#ifndef Natron_Engine_ThreadStorage_h
#define Natron_Engine_ThreadStorage_h

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtCore/QCoreApplication>

// A class that inherits from QThreadStorage, but never sets local data in the main thread.
// It uses an actual instance of the data.
// That way, the ThreadStorage class can be destroyed before leaving the main thread.
//

namespace Natron {

template <class T>
class ThreadStorage : public QThreadStorage<T>
{
public:
    inline bool hasLocalData() const
    {
        return (QThread::currentThread() == qApp->thread() || QThreadStorage<T>::hasLocalData());
    }

    inline T& localData()
    {
        if (QThread::currentThread() == qApp->thread()) {
            return mainData;
        } else {
            return QThreadStorage<T>::localData();
        }
    }

    inline T localData() const
    {
        if (QThread::currentThread() == qApp->thread()) {
            return mainData;
        } else {
            return QThreadStorage<T>::localData();
        }
    }

    inline void setLocalData(T t)
    {
        if (QThread::currentThread() == qApp->thread()) {
            mainData = t;
        } else {
            return QThreadStorage<T>::setLocalData(t);
        }
    }
private:
    T mainData;
};

} // namespace Natron


#endif

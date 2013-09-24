//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_SINGLETON_H_
#define POWITER_ENGINE_SINGLETON_H_

#include <cstdlib> // for std::atexit()
#include <QtCore/QMutexLocker>

// Singleton pattern ( thread-safe) , to have 1 global ptr

template<class T> class Singleton {
    
protected:
    inline explicit Singleton() {
       // _lock = new QMutex;
        Singleton::instance_ = static_cast<T*>(this);
    }
    inline ~Singleton() {
        Singleton::instance_ = 0;
    }
public:
    friend class QMutex;
    
    static T* instance() ;
    static void Destroy();
    static QMutex* _mutex;
    
    
private:
    static T* instance_;
    
    
    inline explicit Singleton(const Singleton&);
    inline Singleton& operator=(const Singleton&){return *this;};
    static T* CreateInstance();
    static void ScheduleForDestruction(void (*)());
    static void DestroyInstance(T*);
    
};

template<class T> T* Singleton<T>::instance() {
    if ( Singleton::instance_ == 0 ) {
        if(_mutex){
            QMutexLocker locker(_mutex);
        }
        if ( Singleton::instance_ == 0 ) {
            Singleton::instance_ = CreateInstance();
            ScheduleForDestruction(Singleton::Destroy);
        }
    }
    return (Singleton::instance_);
}
template<class T> void Singleton<T>::Destroy() {
    if ( Singleton::instance_ != 0 ) {
        DestroyInstance(Singleton::instance_);
        Singleton::instance_ = 0;
    }
}
template<class T> T* Singleton<T>::CreateInstance() {
    return new T();
}
template<class T> void Singleton<T>::ScheduleForDestruction(void (*pFun)()) {
    std::atexit(pFun);
}
template<typename T> void Singleton<T>::DestroyInstance(T* p) {
    delete p;
}

template<class T> T* Singleton<T>::instance_ = 0;
template<class T> QMutex* Singleton<T>::_mutex = 0;


#endif

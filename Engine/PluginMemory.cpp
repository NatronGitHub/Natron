//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "PluginMemory.h"

#include <stdexcept>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QMutex>
CLANG_DIAG_ON(deprecated)
#include "Engine/EffectInstance.h"

PluginMemory::PluginMemory(Natron::EffectInstance* effect)
: _ptr(0)
, _locked(0)
, _nBytes(0)
, _mutex(new QMutex)
, _effect(effect)
{
    _effect->addPluginMemoryPointer(this);
}


PluginMemory::~PluginMemory() {
    delete _mutex;
    delete [] _ptr;
    _effect->removePluginMemoryPointer(this);
}

bool PluginMemory::alloc(size_t nBytes) {
    QMutexLocker l(_mutex);
    if(!_locked){
        _nBytes = nBytes;
        if(_ptr)
            freeMem();
        _ptr = new char[nBytes];
        if (!_ptr) {
            throw std::bad_alloc();
        }
        
        _effect->registerPluginMemory(nBytes);

        return true;
    } else {
        return false;
    }
}

void PluginMemory::freeMem() {
    QMutexLocker l(_mutex);
    _effect->unregisterPluginMemory(_nBytes);
    _nBytes = 0;
    delete [] _ptr;
    _ptr = 0;
    _locked = 0;
}

void* PluginMemory::getPtr() {
    QMutexLocker l(_mutex);
    return _ptr;
}

void PluginMemory::lock() {
    QMutexLocker l(_mutex);
    ++_locked;
}

void PluginMemory::unlock() {
    QMutexLocker l(_mutex);
    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxImageEffectSuiteV1_imageMemoryUnlock
    // "Also note, if you unlock a completely unlocked handle, it has no effect (ie: the lock count can't be negative)."
    if (_locked > 0) {
        --_locked;
    }
}

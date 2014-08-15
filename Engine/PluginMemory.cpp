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


struct PluginMemory::Implementation
{
    std::vector<char> data;
    int     locked;
    QMutex  mutex;
    Natron::EffectInstance* effect;
};

PluginMemory::PluginMemory(Natron::EffectInstance* effect)
: _imp(new Implementation)
{
    _imp->effect = effect;
    _imp->effect->addPluginMemoryPointer(this);
}


PluginMemory::~PluginMemory()
{
    _imp->effect->removePluginMemoryPointer(this);
}

bool
PluginMemory::alloc(size_t nBytes)
{
    QMutexLocker l(&_imp->mutex);
    if (!_imp->locked){
        _imp->data.resize(nBytes);
        _imp->effect->registerPluginMemory(_imp->data.size());

        return true;
    } else {
        return false;
    }
}

void
PluginMemory::freeMem()
{
    QMutexLocker l(&_imp->mutex);
    _imp->effect->unregisterPluginMemory(_imp->data.size());
    _imp->data.clear();
    _imp->locked = 0;
}

void*
PluginMemory::getPtr()
{
    QMutexLocker l(&_imp->mutex);
    return (void*)(_imp->data.data());
}

void
PluginMemory::lock()
{
    QMutexLocker l(&_imp->mutex);
    ++_imp->locked;
}

void
PluginMemory::unlock()
{
    QMutexLocker l(&_imp->mutex);
    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxImageEffectSuiteV1_imageMemoryUnlock
    // "Also note, if you unlock a completely unlocked handle, it has no effect (ie: the lock count can't be negative)."
    if (_imp->locked > 0) {
        --_imp->locked;
    }
}

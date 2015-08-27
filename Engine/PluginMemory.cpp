/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include "PluginMemory.h"

#include <stdexcept>
#include <vector>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QMutex>
CLANG_DIAG_ON(deprecated)
#include "Engine/EffectInstance.h"
#include "Engine/CacheEntry.h"

struct PluginMemory::Implementation
{
    Implementation(Natron::EffectInstance* effect_)
        : data()
          , locked(0)
          , mutex()
          , effect(effect_)
    {
    }

    Natron::RamBuffer<char> data;
    int locked;
    QMutex mutex;
    Natron::EffectInstance* effect;
};

PluginMemory::PluginMemory(Natron::EffectInstance* effect)
    : _imp( new Implementation(effect) )
{
    if (effect) {
        _imp->effect->addPluginMemoryPointer(this);
    }
}

PluginMemory::~PluginMemory()
{
    if (_imp->effect) {
        _imp->effect->removePluginMemoryPointer(this);
    }
}

bool
PluginMemory::alloc(size_t nBytes)
{
    QMutexLocker l(&_imp->mutex);

    if (_imp->locked) {
        return false;
    } else {
        _imp->data.resize(nBytes);
        if (_imp->effect) {
            _imp->effect->registerPluginMemory( _imp->data.size() );
        }

        return true;
    }
}

void
PluginMemory::freeMem()
{
    QMutexLocker l(&_imp->mutex);
    if (_imp->effect) {
        _imp->effect->unregisterPluginMemory( _imp->data.size() );
    }
    _imp->data.clear();
    _imp->locked = 0;
}

void*
PluginMemory::getPtr()
{
    QMutexLocker l(&_imp->mutex);

    assert(_imp->data.size() == 0 || (_imp->data.size() > 0 && _imp->data.getData()));

    return (void*)( _imp->data.getData() );
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


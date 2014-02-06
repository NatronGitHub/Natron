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

#include "OfxMemory.h"

#include <QMutex>

#include "Engine/PluginMemory.h"

OfxMemory::OfxMemory(Natron::EffectInstance* effect)
: OFX::Host::Memory::Instance()
, _memory(new PluginMemory(effect))
{
}

OfxMemory::~OfxMemory() {
    delete _memory;
}

void* OfxMemory::getPtr() {
    return _memory->getPtr();
}

bool OfxMemory::alloc(size_t nBytes) {
    return _memory->alloc(nBytes);
}

void OfxMemory::freeMem() {
    _memory->freeMem();
}

void OfxMemory::lock() {
    _memory->lock();
}

void OfxMemory::unlock() {
    _memory->unlock();
}
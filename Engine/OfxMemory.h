//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#ifndef OFXMEMORY_H
#define OFXMEMORY_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <ofxhImageEffect.h>
#include <ofxhMemory.h>


#include "Global/Macros.h"

namespace Natron {
class EffectInstance;
}
class PluginMemory;
class OfxMemory
    : public OFX::Host::Memory::Instance
{
    PluginMemory* _memory;

public:

    OfxMemory(Natron::EffectInstance* effect);

    virtual ~OfxMemory();

    virtual bool alloc(size_t nBytes) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void freeMem() OVERRIDE FINAL;
    virtual void* getPtr() OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void lock() OVERRIDE FINAL;
    virtual void unlock() OVERRIDE FINAL;
};

#endif // OFXMEMORY_H

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

#ifndef PLUGINMEMORY_H
#define PLUGINMEMORY_H

#include <cstddef>

namespace Natron {
class EffectInstance;
}
class QMutex;

class PluginMemory
{
    char*   _ptr;
    int     _locked;
    size_t _nBytes;
    QMutex* _mutex;
    Natron::EffectInstance* _effect;
    
public:
    
    PluginMemory(Natron::EffectInstance* effect);
    
    ~PluginMemory();
    
    ///throws std::bad_alloc if the allocation failed. Returns false if the memory is already locked.
    ///Returns true on success.
    bool alloc(size_t nBytes);
    
    ///Frees the memory, it doesn't have to be unlocked.
    void freeMem();
    
    void* getPtr();
    
    void lock();
    
    void unlock();
};

#endif // PLUGINMEMORY_H

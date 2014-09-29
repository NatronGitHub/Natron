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

#ifndef PLUGINMEMORY_H
#define PLUGINMEMORY_H

#include <cstddef>
#include <boost/scoped_ptr.hpp>

namespace Natron {
class EffectInstance;
}


class PluginMemory
{
public:

    /**
     * @brief Constructs a new memory chunk that can be used freely and that will be registered and known
     * about by Natron.
     * @param effect If not NULL, it will register the size allocated to the associated node so that Natron
     * can clear this memory when in situation of low memory or when the node is no longer used.
     * On the other hand if the parameter is set to NULL, the memory will not be registered and will live
     * until the plug-in decides to free the memory.
     **/
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

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; //!< PImpl
};

#endif // PLUGINMEMORY_H

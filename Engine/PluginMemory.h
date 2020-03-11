/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef PLUGINMEMORY_H
#define PLUGINMEMORY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstddef>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/ImageStorage.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class PluginMemAllocateMemoryArgs : public AllocateMemoryArgs
{
public:

    std::size_t _nBytes;

    PluginMemAllocateMemoryArgs(std::size_t nBytes)
    : AllocateMemoryArgs()
    , _nBytes(nBytes)
    {
        bitDepth = eImageBitDepthByte;
    }

    virtual ~PluginMemAllocateMemoryArgs()
    {

    }
};

class PluginMemory : public ImageStorageBase
{

public:

    /**
     * @brief Constructs a new memory chunk that can be used freely and that will be registered and known
     * about by Natron.
     **/
    PluginMemory();

    virtual ~PluginMemory();

    char* getData();

    virtual std::size_t getBufferSize() const OVERRIDE FINAL;

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL
    {
        return eStorageModeRAM;
    }

private:

    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) OVERRIDE FINAL;

    virtual void deallocateMemoryImpl() OVERRIDE FINAL;

    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; //!< PImpl
};

NATRON_NAMESPACE_EXIT

#endif // PLUGINMEMORY_H

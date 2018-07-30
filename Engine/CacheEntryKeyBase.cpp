/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "CacheEntryKeyBase.h"

#include <QMutex>
#include <QDebug>
#include "Engine/Hash64.h"


NATRON_NAMESPACE_ENTER

struct CacheEntryKeyBasePrivate
{
    mutable QMutex lock;
    std::string pluginID;
    mutable U64 hash;
    mutable bool hashComputed;

    CacheEntryKeyBasePrivate()
    : lock()
    , pluginID()
    , hash(0)
    , hashComputed(false)
    {

    }
};

CacheEntryKeyBase::CacheEntryKeyBase()
: _imp(new CacheEntryKeyBasePrivate)
{
}

CacheEntryKeyBase::CacheEntryKeyBase(const std::string& pluginID)
: _imp(new CacheEntryKeyBasePrivate)
{
    _imp->pluginID = pluginID;
}


CacheEntryKeyBase::~CacheEntryKeyBase()
{
}



U64
CacheEntryKeyBase::getHash(bool forceComputation) const
{
    QMutexLocker k(&_imp->lock);
    if (!forceComputation && _imp->hashComputed) {
        return _imp->hash;
    }

    Hash64 hash;
    hash.append(getUniqueID());
    appendToHash(&hash);
    hash.computeHash();
    _imp->hash = hash.value();
    _imp->hashComputed = true;
    return _imp->hash;
    
}

void
CacheEntryKeyBase::invalidateHash()
{
    QMutexLocker k(&_imp->lock);
    _imp->hashComputed = false;
}

std::string
CacheEntryKeyBase::hashToString(U64 hash)
{
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string
CacheEntryKeyBase::getHolderPluginID() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->pluginID;
}

void
CacheEntryKeyBase::setHolderPluginID(const std::string& holderID) {
    QMutexLocker k(&_imp->lock);
    _imp->pluginID = holderID;
}

std::size_t
CacheEntryKeyBase::getMetadataSize() const
{
    return 0;
}

void
CacheEntryKeyBase::toMemorySegment(IPCPropertyMap* /*properties*/) const
{

}


CacheEntryKeyBase::FromMemorySegmentRetCodeEnum
CacheEntryKeyBase::fromMemorySegment(const IPCPropertyMap& /*properties*/)

{
    return eFromMemorySegmentRetCodeOk;
}



NATRON_NAMESPACE_EXIT

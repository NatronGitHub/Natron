/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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


#include "EffectInstanceActionResults.h"

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/Hash64.h"

NATRON_NAMESPACE_ENTER;

EffectInstanceActionKeyBase::EffectInstanceActionKeyBase(U64 nodeFrameViewHash,
                                                   const RenderScale& scale,
                                                   const std::string& pluginID)
: _nodeFrameViewHash(nodeFrameViewHash)
, _scale(scale)
{
    setHolderPluginID(pluginID);
}



void
EffectInstanceActionKeyBase::copy(const CacheEntryKeyBase& other)
{
    const EffectInstanceActionKeyBase* o = dynamic_cast<const EffectInstanceActionKeyBase*>(&other);
    if (!o) {
        return;
    }
    CacheEntryKeyBase::copy(other);
    _nodeFrameViewHash = o->_nodeFrameViewHash;
    _scale = o->_scale;
}

bool
EffectInstanceActionKeyBase::equals(const CacheEntryKeyBase& other)
{
    const EffectInstanceActionKeyBase* o = dynamic_cast<const EffectInstanceActionKeyBase*>(&other);
    if (!o) {
        return false;
    }
    if (!CacheEntryKeyBase::equals(other)) {
        return false;
    }

    if (_nodeFrameViewHash != o->_nodeFrameViewHash) {
        return false;
    }

    if (_scale.x != o->_scale.x ||
        _scale.y != o->_scale.y) {
        return false;
    }

    return true;
}

void
EffectInstanceActionKeyBase::appendToHash(Hash64* hash) const
{
    hash->append(_nodeFrameViewHash);
    hash->append(_scale.x);
    hash->append(_scale.y);
}


GetRegionOfDefinitionResults::GetRegionOfDefinitionResults()
: CacheEntryBase(appPTR->getCache())
{

}


GetRegionOfDefinitionResultsPtr
GetRegionOfDefinitionResults::create(const GetRegionOfDefinitionKeyPtr& key)
{
    GetRegionOfDefinitionResultsPtr ret(new GetRegionOfDefinitionResults());
    ret->setKey(key);
    return ret;
}

const RectD&
GetRegionOfDefinitionResults::getRoD() const
{
    return _rod;
}

void
GetRegionOfDefinitionResults::setRoD(const RectD& rod)
{
    assert(getCacheBucketIndex() == -1);
    _rod = rod;
}

std::size_t
GetRegionOfDefinitionResults::getSize() const
{
    return sizeof(RectD);
}

IsIdentityResults::IsIdentityResults()
: CacheEntryBase(appPTR->getCache())
, _identityInputNb(-1)
, _identityTime(0)
, _identityView(0)
{

}

IsIdentityResultsPtr
IsIdentityResults::create(const IsIdentityKeyPtr& key)
{
    IsIdentityResultsPtr ret(new IsIdentityResults());
    ret->setKey(key);
    return ret;

}


void
IsIdentityResults::getIdentityData(int* identityInputNb, TimeValue* identityTime, ViewIdx* identityView) const
{
    *identityInputNb = _identityInputNb;
    *identityTime = _identityTime;
    *identityView = _identityView;
}

void
IsIdentityResults::setIdentityData(int identityInputNb, TimeValue identityTime, ViewIdx identityView)
{
    assert(getCacheBucketIndex() == -1);
    _identityInputNb = identityInputNb;
    _identityTime = identityTime;
    _identityView = identityView;
}

std::size_t
IsIdentityResults::getSize() const
{
    return sizeof(_identityInputNb) + sizeof(_identityTime) + sizeof(_identityView);
}


GetFramesNeededResults::GetFramesNeededResults()
: CacheEntryBase(appPTR->getCache())
, _framesNeeded()
{

}

GetFramesNeededResultsPtr
GetFramesNeededResults::create(const GetFramesNeededKeyPtr& key)
{
    GetFramesNeededResultsPtr ret(new GetFramesNeededResults());
    ret->setKey(key);
    return ret;

}


void
GetFramesNeededResults::getFramesNeeded(FramesNeededMap *framesNeeded) const
{
    *framesNeeded = _framesNeeded;
}

void
GetFramesNeededResults::setFramesNeeded(const FramesNeededMap &framesNeeded)
{
    assert(getCacheBucketIndex() == -1);
    _framesNeeded = framesNeeded;
}

std::size_t
GetFramesNeededResults::getSize() const
{
    return sizeof(_framesNeeded);
}



NATRON_NAMESPACE_EXIT;

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


#include "EffectInstanceActionResults.h"

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/Hash64.h"
#include "Engine/NodeMetadata.h"


namespace bip = boost::interprocess;

NATRON_NAMESPACE_ENTER

EffectInstanceActionKeyBase::EffectInstanceActionKeyBase(U64 nodeTimeViewVariantHash,
                                                         const RenderScale& scale,
                                                         const std::string& pluginID)
: _data()
{
    _data.nodeTimeViewVariantHash = nodeTimeViewVariantHash;
    _data.scale = scale;
    setHolderPluginID(pluginID);
}

void
EffectInstanceActionKeyBase::appendToHash(Hash64* hash) const
{
    hash->append(_data.nodeTimeViewVariantHash);
    hash->append(_data.scale.x);
    hash->append(_data.scale.y);
}

void
EffectInstanceActionKeyBase::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    throw std::runtime_error("EffectInstanceActionKeyBase::toMemorySegment serialization to a persistent cache unimplemented");
} // toMemorySegment

CacheEntryKeyBase::FromMemorySegmentRetCodeEnum
EffectInstanceActionKeyBase::fromMemorySegment(const IPCPropertyMap& /*properties*/)
{
    throw std::runtime_error("EffectInstanceActionKeyBase::fromMemorySegment serialization to a persistent cache unimplemented");
} // fromMemorySegment


GetRegionOfDefinitionResults::GetRegionOfDefinitionResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
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
    _rod = rod;
}

void
GetRegionOfDefinitionResults::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    assert(false);
    throw std::runtime_error("GetRegionOfDefinitionResults::toMemorySegment serialization to a persistent cache unimplemented");
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
GetRegionOfDefinitionResults::fromMemorySegment(bool /*isLockedForWriting*/,
                                                const IPCPropertyMap& /*properties*/)
{
    assert(false);
    throw std::runtime_error("GetRegionOfDefinitionResults::fromMemorySegment serialization to a persistent cache unimplemented");
} // fromMemorySegment


GetDistortionResults::GetDistortionResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
{
    // Distortion results may not be cached in a persistent cache because the plug-in returns to us
    // a pointer to memory it holds.
    assert(!getCache()->isPersistent());
}


GetDistortionResultsPtr
GetDistortionResults::create(const GetDistortionKeyPtr& key)
{
    GetDistortionResultsPtr ret(new GetDistortionResults());
    ret->setKey(key);
    return ret;
}

DistortionFunction2DPtr
GetDistortionResults::getResults() const
{
    return _disto;
}

void
GetDistortionResults::setResults(const DistortionFunction2DPtr& disto)
{
    _disto = disto;
}


void
GetDistortionResults::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    assert(false);
    throw std::runtime_error("GetDistortionResults::toMemorySegment cannot be serialized to a persistent cache");
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
GetDistortionResults::fromMemorySegment(bool /*isLockedForWriting*/,
                                        const IPCPropertyMap& /*properties*/)
{
    assert(false);
    throw std::runtime_error("GetDistortionResults::fromMemorySegment cannot be serialized from a persistent cache");
} // fromMemorySegment

void
IsIdentityKey::appendToHash(Hash64* hash) const
{
    EffectInstanceActionKeyBase::appendToHash(hash);
    hash->append((double)_time);
    Hash64::appendQString(QString::fromUtf8(_plane.getPlaneID().c_str()), hash);
}

IsIdentityResults::IsIdentityResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
, _data()
{
    _data.identityInputNb = -1;
    _data.identityTime = TimeValue(0);
    _data.identityView = ViewIdx(0);
}

IsIdentityResultsPtr
IsIdentityResults::create(const IsIdentityKeyPtr& key)
{
    IsIdentityResultsPtr ret(new IsIdentityResults());
    ret->setKey(key);
    return ret;

}


void
IsIdentityResults::getIdentityData(int* identityInputNb, TimeValue* identityTime, ViewIdx* identityView, ImagePlaneDesc *identityPlane) const
{
    if (identityInputNb) {
        *identityInputNb = _data.identityInputNb;
    }
    if (identityTime) {
        *identityTime = _data.identityTime;
    }
    if (identityView) {
        *identityView = _data.identityView;
    }
    if (identityPlane) {
        *identityPlane = _data.identityPlane;
    }
}

void
IsIdentityResults::setIdentityData(int identityInputNb, TimeValue identityTime, ViewIdx identityView, const ImagePlaneDesc &identityPlane)
{
    _data.identityInputNb = identityInputNb;
    _data.identityTime = identityTime;
    _data.identityView = identityView;
    _data.identityPlane = identityPlane;
}

void
IsIdentityResults::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    assert(false);
    throw std::runtime_error("IsIdentityResults::toMemorySegment serialization to a persistent cache unimplemented");
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
IsIdentityResults::fromMemorySegment(bool /*isLockedForWriting*/,
                                     const IPCPropertyMap& /*properties*/)
{
    assert(false);
    throw std::runtime_error("IsIdentityResults::fromMemorySegment serialization from a persistent cache unimplemented");
} // fromMemorySegment

GetFramesNeededResults::GetFramesNeededResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
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
    _framesNeeded = framesNeeded;
}



void
GetFramesNeededResults::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    assert(false);
    throw std::runtime_error("GetFramesNeededResults::toMemorySegment serialization to a persistent cache unimplemented");
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
GetFramesNeededResults::fromMemorySegment(bool /*isLockedForWriting*/,
                                          const IPCPropertyMap& /*properties*/)
{
    assert(false);
    throw std::runtime_error("GetFramesNeededResults::fromMemorySegment serialization from a persistent cache unimplemented");
} // fromMemorySegment



GetFrameRangeResults::GetFrameRangeResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
, _range()
{
    _range.min = _range.max = 0;
}

GetFrameRangeResultsPtr
GetFrameRangeResults::create(const GetFrameRangeKeyPtr& key)
{
    GetFrameRangeResultsPtr ret(new GetFrameRangeResults());
    ret->setKey(key);
    return ret;

}


void
GetFrameRangeResults::getFrameRangeResults(RangeD *range) const
{
    *range = _range;
}

void
GetFrameRangeResults::setFrameRangeResults(const RangeD &range)
{
    _range = range;
}

void
GetFrameRangeResults::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    assert(false);
    throw std::runtime_error("GetFrameRangeResults::toMemorySegment serialization to a persistent cache unimplemented");
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
GetFrameRangeResults::fromMemorySegment(bool /*isLockedForWriting*/,
                                        const IPCPropertyMap& /*properties*/)
{
    assert(false);
    throw std::runtime_error("GetFrameRangeResults::fromMemorySegment serialization from a persistent cache unimplemented");
} // fromMemorySegment



GetTimeInvariantMetadataResults::GetTimeInvariantMetadataResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
, _metadata()
{

}

GetTimeInvariantMetadataResultsPtr
GetTimeInvariantMetadataResults::create(const GetTimeInvariantMetadataKeyPtr& key)
{
    GetTimeInvariantMetadataResultsPtr ret(new GetTimeInvariantMetadataResults());
    ret->setKey(key);
    return ret;

}


const NodeMetadataPtr&
GetTimeInvariantMetadataResults::getMetadataResults() const
{
    return _metadata;
}

void
GetTimeInvariantMetadataResults::setMetadataResults(const NodeMetadataPtr &metadata)
{
    _metadata = metadata;
}


void
GetTimeInvariantMetadataResults::toMemorySegment(IPCPropertyMap* properties) const
{
    assert(_metadata);
    _metadata->toMemorySegment(properties);
    CacheEntryBase::toMemorySegment(properties);
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
GetTimeInvariantMetadataResults::fromMemorySegment(bool isLockedForWriting,
                                                    const IPCPropertyMap& properties)
{
    assert(_metadata);
    _metadata->fromMemorySegment(properties);
    return CacheEntryBase::fromMemorySegment(isLockedForWriting, properties);
} // fromMemorySegment



GetComponentsResults::GetComponentsResults()
: CacheEntryBase(appPTR->getGeneralPurposeCache())
, _neededInputLayers()
, _producedLayers()
, _passThroughPlanes()
, _data()
{
    _data.passThroughInputNb = -1;
    _data.passThroughTime = TimeValue(0);
    _data.passThroughView = ViewIdx(0);
    _data.processAllLayers = false;
    _data.doR = _data.doG = _data.doB = _data.doA = true;

}

GetComponentsResultsPtr
GetComponentsResults::create(const GetComponentsKeyPtr& key)
{
    GetComponentsResultsPtr ret(new GetComponentsResults());
    ret->setKey(key);
    return ret;

}

const std::map<int, std::list<ImagePlaneDesc> >&
GetComponentsResults::getNeededInputPlanes() const
{
    return _neededInputLayers;
}

const std::list<ImagePlaneDesc>&
GetComponentsResults::getProducedPlanes() const
{
    return _producedLayers;
}

std::bitset<4>
GetComponentsResults::getProcessChannels() const
{
    std::bitset<4> ret;
    ret[0] = _data.doR;
    ret[1] = _data.doG;
    ret[2] = _data.doB;
    ret[3] = _data.doA;
    return ret;
}

void
GetComponentsResults::getResults(std::map<int, std::list<ImagePlaneDesc> >* neededInputLayers,
                                 std::list<ImagePlaneDesc>* producedLayers,
                                 std::list<ImagePlaneDesc>* passThroughPlanes,
                                 int *passThroughInputNb,
                                 TimeValue *passThroughTime,
                                 ViewIdx *passThroughView,
                                 std::bitset<4> *processChannels,
                                 bool *processAllLayers) const
{
    if (neededInputLayers) {
        *neededInputLayers = _neededInputLayers;
    }
    if (producedLayers) {
        *producedLayers = _producedLayers;
    }
    if (passThroughPlanes) {
        *passThroughPlanes = _passThroughPlanes;
    }
    if (passThroughInputNb) {
        *passThroughInputNb = _data.passThroughInputNb;
    }
    if (passThroughTime) {
        *passThroughTime = _data.passThroughTime;
    }
    if (passThroughView) {
        *passThroughView = _data.passThroughView;
    }
    if (processChannels) {
        (*processChannels)[0] = _data.doR;
        (*processChannels)[1] = _data.doG;
        (*processChannels)[2] = _data.doB;
        (*processChannels)[3] = _data.doA;
    }
    if (processAllLayers) {
        *processAllLayers = _data.processAllLayers;
    }
}

void
GetComponentsResults::setResults(const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers,
                                 const std::list<ImagePlaneDesc>& producedLayers,
                                 const std::list<ImagePlaneDesc>& passThroughPlanes,
                                 int passThroughInputNb,
                                 TimeValue passThroughTime,
                                 ViewIdx passThroughView,
                                 std::bitset<4> processChannels,
                                 bool processAllLayers)
{
    _neededInputLayers = neededInputLayers;
    _producedLayers = producedLayers;
    _passThroughPlanes = passThroughPlanes;
    _data.passThroughInputNb = passThroughInputNb;
    _data.passThroughTime = passThroughTime;
    _data.passThroughView = passThroughView;
    _data.doR = processChannels[0];
    _data.doG = processChannels[1];
    _data.doB = processChannels[2];
    _data.doA = processChannels[3];
    _data.processAllLayers = processAllLayers;
}



void
GetComponentsResults::toMemorySegment(IPCPropertyMap* /*properties*/) const
{
    assert(false);
    throw std::runtime_error("GetComponentsResults::toMemorySegment serialization to a persistent cache unimplemented");
} // toMemorySegment

CacheEntryBase::FromMemorySegmentRetCodeEnum
GetComponentsResults::fromMemorySegment(bool /*isLockedForWriting*/,
                                          const IPCPropertyMap& /*properties*/) {
    assert(false);
    throw std::runtime_error("GetComponentsResults::fromMemorySegment serialization from a persistent cache unimplemented");
} // fromMemorySegment


NATRON_NAMESPACE_EXIT

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
#include <boost/foreach.hpp>
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/Hash64.h"
#include "Engine/NodeMetadata.h"


namespace bip = boost::interprocess;

NATRON_NAMESPACE_ENTER;

EffectInstanceActionKeyBase::EffectInstanceActionKeyBase(U64 nodeTimeInvariantHash,
                                                         TimeValue time,
                                                         ViewIdx view,
                                                         const RenderScale& scale,
                                                         const std::string& pluginID)
: _nodeTimeInvariantHash(nodeTimeInvariantHash)
, _time(time)
, _view(view)
, _scale(scale)
{
    setHolderPluginID(pluginID);
}

void
EffectInstanceActionKeyBase::appendToHash(Hash64* hash) const
{
    hash->append(_nodeTimeInvariantHash);
    hash->append((double)_time);
    hash->append((int)_view);
    hash->append(_scale.x);
    hash->append(_scale.y);
}

void
EffectInstanceActionKeyBase::toMemorySegment(ExternalSegmentType* segment) const
{
    writeMMObject(_nodeTimeInvariantHash, "hash", segment);
    writeMMObject(_time, "time", segment);
    writeMMObject(_view, "view", segment);
    writeMMObject(_scale.x, "sx", segment);
    writeMMObject(_scale.y, "sy", segment);
    CacheEntryKeyBase::toMemorySegment(segment);
} // toMemorySegment

void
EffectInstanceActionKeyBase::fromMemorySegment(ExternalSegmentType* segment)
{
    readMMObject("hash", segment, &_nodeTimeInvariantHash);
    readMMObject("time", segment, &_time);
    readMMObject("view", segment, &_view);
    readMMObject("sx", segment, &_scale.x);
    readMMObject("sy", segment, &_scale.y);
    CacheEntryKeyBase::fromMemorySegment(segment);
} // fromMemorySegment

std::size_t
EffectInstanceActionKeyBase::getMetadataSize() const
{
    std::size_t ret = CacheEntryKeyBase::getMetadataSize();
    ret += sizeof(_nodeTimeInvariantHash);
    ret += sizeof(_time);
    ret += sizeof(_view);
    ret += sizeof(_scale);
    return ret;
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
    _rod = rod;
}

std::size_t
GetRegionOfDefinitionResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    ret += sizeof(_rod);
    return ret;
}


void
GetRegionOfDefinitionResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    writeMMObject(_rod, "rod", segment);
    CacheEntryBase::toMemorySegment(segment, tileDataPtr);
} // toMemorySegment

void
GetRegionOfDefinitionResults::fromMemorySegment(ExternalSegmentType* segment, const void* tileDataPtr)
{
    readMMObject("rod", segment, &_rod);
    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);
} // fromMemorySegment

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
    _identityInputNb = identityInputNb;
    _identityTime = identityTime;
    _identityView = identityView;
}

std::size_t
IsIdentityResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    ret += sizeof(_identityInputNb);
    ret += sizeof(_identityTime);
    ret += sizeof(_identityView);
    return ret;
}

void
IsIdentityResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    writeMMObject(_identityInputNb, "inputNb", segment);
    writeMMObject(_identityTime, "inputTime", segment);
    writeMMObject(_identityView, "inputView", segment);
    CacheEntryBase::toMemorySegment(segment, tileDataPtr);
} // toMemorySegment

void
IsIdentityResults::fromMemorySegment(ExternalSegmentType* segment, const void* tileDataPtr)
{
    readMMObject("inputNb", segment, &_identityInputNb);
    readMMObject("inputTime", segment, &_identityTime);
    readMMObject("inputView", segment, &_identityView);
    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);
} // fromMemorySegment

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
    _framesNeeded = framesNeeded;
}

std::size_t
GetFramesNeededResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    // Hint a fake size, that's enough to ensure the memory allocation is ok
    ret += 1024;
    return ret;
}

// This is made complicated so it can be inserted in interprocess data structures.
typedef bip::allocator<void, ExternalSegmentType::segment_manager> void_allocator;
typedef bip::allocator<ViewIdx, ExternalSegmentType::segment_manager> ViewIdx_allocator;
typedef bip::allocator<RangeD, ExternalSegmentType::segment_manager> RangeD_allocator;

typedef bip::vector<RangeD, RangeD_allocator> RangeDVector_ExternalSegment;

typedef bip::allocator<RangeDVector_ExternalSegment, ExternalSegmentType::segment_manager> RangeDVector_allocator;

typedef bip::map<ViewIdx, RangeDVector_ExternalSegment, std::less<ViewIdx>, RangeDVector_allocator> FrameRangesMap_ExternalSegment;

typedef std::pair<ViewIdx, RangeDVector_ExternalSegment> FrameRangesMap_value_type;

typedef bip::allocator<FrameRangesMap_ExternalSegment, ExternalSegmentType::segment_manager> FrameRangesMap_ExternalSegment_allocator;

typedef bip::map<int, FrameRangesMap_ExternalSegment, std::less<int>, FrameRangesMap_ExternalSegment_allocator> FramesNeededMap_ExternalSegment;

typedef std::pair<int, FrameRangesMap_ExternalSegment> FramesNeededMap_value_type;

typedef bip::allocator<FramesNeededMap_ExternalSegment, ExternalSegmentType::segment_manager> FramesNeededMap_ExternalSegment_allocator;

void
GetFramesNeededResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    // An allocator convertible to any allocator<T, segment_manager_t> type
    void_allocator alloc_inst (segment->get_segment_manager());

    FramesNeededMap_ExternalSegment* externalMap = segment->construct<FramesNeededMap_ExternalSegment>("framesNeeded")(alloc_inst);

    for (FramesNeededMap::const_iterator it = _framesNeeded.begin(); it != _framesNeeded.end(); ++it) {

        FrameRangesMap_ExternalSegment extFrameRangeMap(alloc_inst);

        for (FrameRangesMap::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it) {

            RangeDVector_ExternalSegment extRangeVec(alloc_inst);
            for (std::size_t i = 0; i < it2->second.size(); ++i) {
                extRangeVec.push_back(it2->second[i]);
            }
            FrameRangesMap_value_type v = std::make_pair(it2->first, extRangeVec);
            extFrameRangeMap.insert(v);
        }

        FramesNeededMap_value_type v = std::make_pair(it->first, extFrameRangeMap);
        externalMap->insert(v);
    }
    
    writeMMObject(externalMap, "framesNeeded", segment);
    CacheEntryBase::toMemorySegment(segment, tileDataPtr);
} // toMemorySegment

void
GetFramesNeededResults::fromMemorySegment(ExternalSegmentType* segment, const void* tileDataPtr)
{
    FramesNeededMap_ExternalSegment externalMap;
    readMMObject("framesNeeded", segment, &externalMap);
    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);


    for (FramesNeededMap_ExternalSegment::iterator it = externalMap.begin(); it != externalMap.end(); ++it) {
        FrameRangesMap& frameRnageMap = _framesNeeded[it->first];
        for (FrameRangesMap_ExternalSegment::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

        }
    }
} // fromMemorySegment


GetDistorsionResults::GetDistorsionResults()
: CacheEntryBase(appPTR->getCache())
, _distorsion()
{

}

GetDistorsionResultsPtr
GetDistorsionResults::create(const GetDistorsionKeyPtr& key)
{
    GetDistorsionResultsPtr ret(new GetDistorsionResults());
    ret->setKey(key);
    return ret;

}


DistorsionFunction2DPtr
GetDistorsionResults::getDistorsionResults() const
{
    return _distorsion;
}

void
GetDistorsionResults::setDistorsionResults(const DistorsionFunction2DPtr &disto)
{
    _distorsion = disto;
}

std::size_t
GetDistorsionResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    if (!_distorsion) {
        return ret;
    }

    ret += (std::size_t)_distorsion->customDataSizeHintInBytes;
    return ret;
}



GetFrameRangeResults::GetFrameRangeResults()
: CacheEntryBase(appPTR->getCache())
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

std::size_t
GetFrameRangeResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    ret += sizeof(_range);
    return ret;
}


GetTimeInvariantMetaDatasResults::GetTimeInvariantMetaDatasResults()
: CacheEntryBase(appPTR->getCache())
, _metadatas()
{

}

GetTimeInvariantMetaDatasResultsPtr
GetTimeInvariantMetaDatasResults::create(const GetTimeInvariantMetaDatasKeyPtr& key)
{
    GetTimeInvariantMetaDatasResultsPtr ret(new GetTimeInvariantMetaDatasResults());
    ret->setKey(key);
    return ret;

}


const NodeMetadataPtr&
GetTimeInvariantMetaDatasResults::getMetadatasResults() const
{
    return _metadatas;
}

void
GetTimeInvariantMetaDatasResults::setMetadatasResults(const NodeMetadataPtr &metadatas)
{
    _metadatas = metadatas;
}

std::size_t
GetTimeInvariantMetaDatasResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    // Hint a fake size
    ret += 1024;
    return ret;
}


GetComponentsResults::GetComponentsResults()
: CacheEntryBase(appPTR->getCache())
, _neededInputLayers()
, _producedLayers()
, _passThroughPlanes()
, _passThroughInputNb(-1)
, _passThroughTime(0)
, _passThroughView(0)
, _processChannels()
, _processAllLayers(false)
{

}

GetComponentsResultsPtr
GetComponentsResults::create(const GetComponentsKeyPtr& key)
{
    GetComponentsResultsPtr ret(new GetComponentsResults());
    ret->setKey(key);
    return ret;

}


void
GetComponentsResults::getResults(std::map<int, std::list<ImageComponents> >* neededInputLayers,
                                 std::list<ImageComponents>* producedLayers,
                                 std::list<ImageComponents>* passThroughPlanes,
                                 int *passThroughInputNb,
                                 TimeValue *passThroughTime,
                                 ViewIdx *passThroughView,
                                 std::bitset<4> *processChannels,
                                 bool *processAllLayers) const
{
    *neededInputLayers = _neededInputLayers;
    *producedLayers = _producedLayers;
    *passThroughPlanes = _passThroughPlanes;
    *passThroughInputNb = _passThroughInputNb;
    *passThroughTime = _passThroughTime;
    *passThroughView = _passThroughView;
    *processChannels = _processChannels;
    *processAllLayers = _processAllLayers;
}

void
GetComponentsResults::setResults(const std::map<int, std::list<ImageComponents> >& neededInputLayers,
                                 const std::list<ImageComponents>& producedLayers,
                                 const std::list<ImageComponents>& passThroughPlanes,
                                 int passThroughInputNb,
                                 TimeValue passThroughTime,
                                 ViewIdx passThroughView,
                                 std::bitset<4> processChannels,
                                 bool processAllLayers)
{
    _neededInputLayers = neededInputLayers;
    _producedLayers = producedLayers;
    _passThroughPlanes = passThroughPlanes;
    _passThroughInputNb = passThroughInputNb;
    _passThroughTime = passThroughTime;
    _passThroughView = passThroughView;
    _processChannels = processChannels;
    _processAllLayers = processAllLayers;
}

std::size_t
GetComponentsResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    // Hint a fake size
    ret += 1024;
    return ret;
}
NATRON_NAMESPACE_EXIT;

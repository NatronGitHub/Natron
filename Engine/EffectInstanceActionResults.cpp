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

// A vector of RangeD
typedef bip::vector<RangeD, RangeD_allocator> RangeDVector_ExternalSegment;

typedef bip::allocator<RangeDVector_ExternalSegment, ExternalSegmentType::segment_manager> RangeDVector_allocator;

typedef std::pair<ViewIdx, RangeDVector_ExternalSegment> FrameRangesMap_value_type;

typedef bip::allocator<FrameRangesMap_value_type, ExternalSegmentType::segment_manager> FrameRangesMap_value_type_allocator;

// A map<ViewIdx, vector<RangeD> >
typedef bip::map<ViewIdx, RangeDVector_ExternalSegment, std::less<ViewIdx>, FrameRangesMap_value_type_allocator> FrameRangesMap_ExternalSegment;

typedef std::pair<int, FrameRangesMap_ExternalSegment> FramesNeededMap_value_type;

typedef bip::allocator<FramesNeededMap_value_type, ExternalSegmentType::segment_manager> FramesNeededMap_value_type_allocator;

// A map<int,  map<ViewIdx, vector<RangeD> > >
typedef bip::map<int, FrameRangesMap_ExternalSegment, std::less<int>, FramesNeededMap_value_type_allocator> FramesNeededMap_ExternalSegment;


typedef bip::allocator<FramesNeededMap_ExternalSegment, ExternalSegmentType::segment_manager> FramesNeededMap_ExternalSegment_allocator;

void
GetFramesNeededResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    // An allocator convertible to any allocator<T, segment_manager_t> type
    void_allocator alloc_inst (segment->get_segment_manager());

    FramesNeededMap_ExternalSegment* externalMap = segment->construct<FramesNeededMap_ExternalSegment>("framesNeeded")(alloc_inst);
    if (!externalMap) {
        throw std::bad_alloc();
    }
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
    FramesNeededMap_ExternalSegment *externalMap = segment->find<FramesNeededMap_ExternalSegment>("framesNeeded").first;
    if (!externalMap) {
        throw std::bad_alloc();
    }
    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);


    for (FramesNeededMap_ExternalSegment::iterator it = externalMap->begin(); it != externalMap->end(); ++it) {
        FrameRangesMap& frameRangeMap = _framesNeeded[it->first];
        for (FrameRangesMap_ExternalSegment::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            std::vector<RangeD> &rangeVec = frameRangeMap[it2->first];

            for (std::size_t i = 0; i < it2->second.size(); ++i) {
                rangeVec.push_back(it2->second[i]);
            }
        }
    }
} // fromMemorySegment



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


void
GetFrameRangeResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    writeMMObject(_range, "range", segment);
    CacheEntryBase::toMemorySegment(segment, tileDataPtr);
} // toMemorySegment

void
GetFrameRangeResults::fromMemorySegment(ExternalSegmentType* segment, const void* tileDataPtr)
{
    readMMObject("range", segment, &_range);
    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);
} // fromMemorySegment



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


void
GetTimeInvariantMetaDatasResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    _metadatas->toMemorySegment(segment);
    CacheEntryBase::toMemorySegment(segment, tileDataPtr);
} // toMemorySegment

void
GetTimeInvariantMetaDatasResults::fromMemorySegment(ExternalSegmentType* segment, const void* tileDataPtr)
{
    _metadatas->fromMemorySegment(segment);
    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);
} // fromMemorySegment



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

typedef bip::allocator<String_ExternalSegment, ExternalSegmentType::segment_manager> String_ExternalSegment_allocator;
typedef bip::vector<String_ExternalSegment, String_ExternalSegment_allocator> StringVector_ExternalSegment;

// A simplified version of ImageComponents class that can go in shared memory
class MM_ImageComponents
{

public:

    String_ExternalSegment layerName, componentsName;
    StringVector_ExternalSegment channels;

    MM_ImageComponents(const CharAllocator_ExternalSegment& charAllocator, const String_ExternalSegment_allocator& stringAllocator)
    : layerName(charAllocator)
    , componentsName(stringAllocator)
    , channels()
    {

    }
};

typedef bip::allocator<MM_ImageComponents, ExternalSegmentType::segment_manager> ImageComponents_ExternalSegment_allocator;
typedef bip::vector<MM_ImageComponents, ImageComponents_ExternalSegment_allocator> ImageComponentsVector_ExternalSegment;

typedef std::pair<int, ImageComponentsVector_ExternalSegment> NeededInputLayersValueType;
typedef bip::allocator<NeededInputLayersValueType, ExternalSegmentType::segment_manager> NeededInputLayersValueType_allocator;

typedef bip::map<int, ImageComponentsVector_ExternalSegment, std::less<int>, NeededInputLayersValueType_allocator> NeededInputLayersMap_ExternalSegment;

static void imageComponentsListToSharedMemoryComponentsList(const void_allocator& allocator, const std::list<ImageComponents>& inComps, ImageComponentsVector_ExternalSegment* outComps)
{
    for (std::list<ImageComponents>::const_iterator it = inComps.begin() ; it != inComps.end(); ++it) {
        MM_ImageComponents comps(allocator, allocator);
        outComps->push_back(comps);
    }
}

static void imageComponentsListFromSharedMemoryComponentsList(const ImageComponentsVector_ExternalSegment& inComps, std::list<ImageComponents>* outComps)
{
    for (ImageComponentsVector_ExternalSegment::const_iterator it = inComps.begin() ; it != inComps.end(); ++it) {
        std::string layerName(it->layerName.c_str());
        std::string compsName(it->componentsName.c_str());
        std::vector<std::string> channels(it->channels.size());

        int i = 0;
        for (StringVector_ExternalSegment::const_iterator it2 = it->channels.begin(); it2 != it->channels.end(); ++it2, ++i) {
            channels[i] = std::string(it2->c_str());
        }
        ImageComponents c(layerName, compsName, channels);
        outComps->push_back(c);
    }
}

void
GetComponentsResults::toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const
{
    // An allocator convertible to any allocator<T, segment_manager_t> type
    void_allocator alloc_inst(segment->get_segment_manager());

    {
        NeededInputLayersMap_ExternalSegment *neededLayers = segment->construct<NeededInputLayersMap_ExternalSegment>("neededInputLayers")(alloc_inst);
        if (!neededLayers) {
            throw std::bad_alloc();
        }

        for (std::map<int, std::list<ImageComponents> >::const_iterator it = _neededInputLayers.begin(); it != _neededInputLayers.end(); ++it) {
            ImageComponentsVector_ExternalSegment vec(alloc_inst);
            imageComponentsListToSharedMemoryComponentsList(alloc_inst, it->second, &vec);

            NeededInputLayersValueType v = std::make_pair(it->first, vec);
            neededLayers->insert(v);
        }
    }
    {
        ImageComponentsVector_ExternalSegment *producedLayers = segment->construct<ImageComponentsVector_ExternalSegment>("producedLayers")(alloc_inst);
        if (!producedLayers) {
            throw std::bad_alloc();
        }
        imageComponentsListToSharedMemoryComponentsList(alloc_inst, _producedLayers, producedLayers);
    }
    {
        ImageComponentsVector_ExternalSegment *ptPlanes = segment->construct<ImageComponentsVector_ExternalSegment>("passThroughLayers")(alloc_inst);
        if (!ptPlanes) {
            throw std::bad_alloc();
        }
        imageComponentsListToSharedMemoryComponentsList(alloc_inst, _passThroughPlanes, ptPlanes);
    }

    writeMMObject(_passThroughTime, "passThroughTime", segment);
    writeMMObject(_passThroughView, "passThroughView", segment);
    writeMMObject(_passThroughInputNb, "passThroughInputNb", segment);
    writeMMObject(_processAllLayers, "processAll", segment);

    bool doR = _processChannels[0];
    writeMMObject(doR, "doR", segment);

    bool doG = _processChannels[1];
    writeMMObject(doG, "doG", segment);

    bool doB = _processChannels[2];
    writeMMObject(doB, "doB", segment);

    bool doA = _processChannels[3];
    writeMMObject(doA, "doA", segment);

    CacheEntryBase::toMemorySegment(segment, tileDataPtr);
} // toMemorySegment

void
GetComponentsResults::fromMemorySegment(ExternalSegmentType* segment, const void* tileDataPtr)
{
    {
        NeededInputLayersMap_ExternalSegment *neededLayers = segment->find<NeededInputLayersMap_ExternalSegment>("neededInputLayers").first;
        if (!neededLayers) {
            throw std::bad_alloc();
        }
        for (NeededInputLayersMap_ExternalSegment::const_iterator it = neededLayers->begin(); it != neededLayers->end(); ++it) {
            std::list<ImageComponents>& comps = _neededInputLayers[it->first];
            imageComponentsListFromSharedMemoryComponentsList(it->second, &comps);
        }
    }
    {
        ImageComponentsVector_ExternalSegment* producedLayers = segment->find<ImageComponentsVector_ExternalSegment>("producedLayers").first;
        if (!producedLayers) {
            throw std::bad_alloc();
        }
        imageComponentsListFromSharedMemoryComponentsList(*producedLayers, &_producedLayers);
    }
    {
        ImageComponentsVector_ExternalSegment* ptLayers = segment->find<ImageComponentsVector_ExternalSegment>("passThroughLayers").first;
        if (!ptLayers) {
            throw std::bad_alloc();
        }
        imageComponentsListFromSharedMemoryComponentsList(*ptLayers, &_passThroughPlanes);
    }
    readMMObject("passThroughTime", segment, &_passThroughTime);
    readMMObject("passThroughView", segment, &_passThroughView);
    readMMObject("passThroughInputNb", segment, &_passThroughInputNb);
    readMMObject("processAll", segment, &_processAllLayers);

    bool doR;
    readMMObject("doR", segment, &doR);
    _processChannels[0] = doR;

    bool doG;
    readMMObject("doG", segment, &doG);
    _processChannels[1] = doG;

    bool doB;
    readMMObject("doB", segment, &doB);
    _processChannels[2] = doB;

    bool doA;
    readMMObject("doA", segment, &doA);
    _processChannels[3] = doA;


    CacheEntryBase::fromMemorySegment(segment, tileDataPtr);
} // fromMemorySegment


NATRON_NAMESPACE_EXIT;

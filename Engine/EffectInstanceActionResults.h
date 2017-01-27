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

#ifndef NATRON_ENGINE_EFFECTINSTANCEACTIONRESULTS_H
#define NATRON_ENGINE_EFFECTINSTANCEACTIONRESULTS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <bitset>
#include <map>
#include <list>
#include <vector>

#include "Global/GlobalDefines.h"

#include "Engine/CacheEntryBase.h"
#include "Engine/Distortion2D.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/RectD.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;


class EffectInstanceActionKeyBase : public CacheEntryKeyBase
{
public:

    EffectInstanceActionKeyBase(U64 nodeTimeInvariantHash,
                                TimeValue time,
                                ViewIdx view,
                                const RenderScale& scale,
                                const std::string& pluginID);

    virtual ~EffectInstanceActionKeyBase()
    {
        
    }

    virtual TimeValue getTime() const OVERRIDE FINAL
    {
        return _data.time;
    }

    virtual ViewIdx getView() const OVERRIDE FINAL
    {
        return _data.view;
    }

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const OVERRIDE;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix) OVERRIDE;

    virtual std::size_t getMetadataSize() const OVERRIDE;

private:

    virtual void appendToHash(Hash64* hash) const OVERRIDE FINAL;

    struct KeyShmData
    {
        U64 nodeTimeInvariantHash;
        TimeValue time;
        ViewIdx view;
        RenderScale scale;
    };

    KeyShmData _data;
};

class GetRegionOfDefinitionKey : public EffectInstanceActionKeyBase
{
public:

    GetRegionOfDefinitionKey(U64 nodeTimeInvariantHash,
                             TimeValue time,
                             ViewIdx view,
                             const RenderScale& scale,
                             const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash, time, view, scale, pluginID)
    {

    }

    virtual ~GetRegionOfDefinitionKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetRoDResults;
    }



};


class GetRegionOfDefinitionResults : public CacheEntryBase
{
    GetRegionOfDefinitionResults();

public:

    static GetRegionOfDefinitionResultsPtr create(const GetRegionOfDefinitionKeyPtr& key);

    virtual ~GetRegionOfDefinitionResults()
    {
        
    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    const RectD& getRoD() const;
    void setRoD(const RectD& rod);

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr) OVERRIDE FINAL;


private:

    RectD _rod;

};

inline GetRegionOfDefinitionResultsPtr
toGetRegionOfDefinitionResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetRegionOfDefinitionResults>(entry);
}



class IsIdentityKey : public EffectInstanceActionKeyBase
{
public:

    IsIdentityKey(U64 nodeTimeInvariantHash,
                  TimeValue time,
                  ViewIdx view,
                  const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash, time, view, RenderScale(1.), pluginID)
    {

    }

    virtual ~IsIdentityKey()
    {

    }


    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDIsIdentityResults;
    }


    
};


class IsIdentityResults : public CacheEntryBase
{
    IsIdentityResults();

public:

    static IsIdentityResultsPtr create(const IsIdentityKeyPtr& key);

    virtual ~IsIdentityResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    void getIdentityData(int* identityInputNb, TimeValue* identityTime, ViewIdx* identityView) const;
    void setIdentityData(int identityInputNb, TimeValue identityTime, ViewIdx identityView);

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr) OVERRIDE FINAL;

private:

    struct ShmData
    {
        int identityInputNb;
        TimeValue identityTime;
        ViewIdx identityView;
    };
    ShmData _data;

};

inline IsIdentityResultsPtr
toIsIdentityResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<IsIdentityResults>(entry);
}



class GetFramesNeededKey : public EffectInstanceActionKeyBase
{
public:

    GetFramesNeededKey(U64 nodeTimeInvariantHash,
                       TimeValue time,
                       ViewIdx view,
                       const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash, time, view, RenderScale(1.), pluginID)
    {
        
    }
    
    virtual ~GetFramesNeededKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDFramesNeededResults;
    }


};

typedef std::map<ViewIdx, std::vector<RangeD> > FrameRangesMap;
typedef std::map<int, FrameRangesMap> FramesNeededMap;


class GetFramesNeededResults : public CacheEntryBase
{
    GetFramesNeededResults();

public:

    static GetFramesNeededResultsPtr create(const GetFramesNeededKeyPtr& key);

    virtual ~GetFramesNeededResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    void getFramesNeeded(FramesNeededMap* framesNeeded) const;
    void setFramesNeeded(const FramesNeededMap& framesNeeded);

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr) OVERRIDE FINAL;

private:

    FramesNeededMap _framesNeeded;

};

inline GetFramesNeededResultsPtr
toGetFramesNeededResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetFramesNeededResults>(entry);
}


class GetFrameRangeKey : public EffectInstanceActionKeyBase
{
public:

    GetFrameRangeKey(U64 nodeTimeInvariantHash,
                     const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash, TimeValue(0.), ViewIdx(0), RenderScale(1.), pluginID)
    {

    }

    virtual ~GetFrameRangeKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetFrameRangeResults;
    }
};


class GetFrameRangeResults : public CacheEntryBase
{
    GetFrameRangeResults();

public:

    static GetFrameRangeResultsPtr create(const GetFrameRangeKeyPtr& key);

    virtual ~GetFrameRangeResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    void getFrameRangeResults(RangeD* range) const;
    void setFrameRangeResults(const RangeD& range);

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr) OVERRIDE FINAL;

private:

    RangeD _range;

};

inline GetFrameRangeResultsPtr
toGetFrameRangeResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetFrameRangeResults>(entry);
}


class GetTimeInvariantMetaDatasKey : public EffectInstanceActionKeyBase
{
public:

    GetTimeInvariantMetaDatasKey(U64 nodeTimeViewInvariantHash,
                                 const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewInvariantHash, TimeValue(0.), ViewIdx(0), RenderScale(1.), pluginID)
    {

    }

    virtual ~GetTimeInvariantMetaDatasKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetTimeInvariantMetaDatasResults;
    }
};


class GetTimeInvariantMetaDatasResults : public CacheEntryBase
{
    GetTimeInvariantMetaDatasResults();

public:

    static GetTimeInvariantMetaDatasResultsPtr create(const GetTimeInvariantMetaDatasKeyPtr& key);

    virtual ~GetTimeInvariantMetaDatasResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    const NodeMetadataPtr& getMetadatasResults() const;
    void setMetadatasResults(const NodeMetadataPtr& metadatas);

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr) OVERRIDE FINAL;

private:

    NodeMetadataPtr _metadatas;

};

inline GetTimeInvariantMetaDatasResultsPtr
toGetTimeInvariantMetaDatasResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetTimeInvariantMetaDatasResults>(entry);
}


class GetComponentsKey : public EffectInstanceActionKeyBase
{
public:

    GetComponentsKey(U64 nodeTimeInvariantHash,
                     TimeValue time,
                     ViewIdx view,
                     const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash, time, view, RenderScale(1.), pluginID)
    {

    }

    virtual ~GetComponentsKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetComponentsResults;
    }

};


class GetComponentsResults : public CacheEntryBase
{
    GetComponentsResults();

public:

    static GetComponentsResultsPtr create(const GetComponentsKeyPtr& key);

    virtual ~GetComponentsResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    void getResults(std::map<int, std::list<ImagePlaneDesc> >* neededInputLayers,
                    std::list<ImagePlaneDesc>* producedLayers,
                    std::list<ImagePlaneDesc>* passThroughPlanes,
                    int *passThroughInputNb,
                    TimeValue *passThroughTime,
                    ViewIdx *passThroughView,
                    std::bitset<4> *processChannels,
                    bool *processAllLayers) const;

    void setResults(const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers,
                    const std::list<ImagePlaneDesc>& producedLayers,
                    const std::list<ImagePlaneDesc>& passThroughPlanes,
                    int passThroughInputNb,
                    TimeValue passThroughTime,
                    ViewIdx passThroughView,
                    std::bitset<4> processChannels,
                    bool processAllLayers);


    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr) OVERRIDE FINAL;
    
private:

    std::map<int, std::list<ImagePlaneDesc> > _neededInputLayers;
    std::list<ImagePlaneDesc> _producedLayers;
    std::list<ImagePlaneDesc> _passThroughPlanes;

    struct ShmData
    {
        int passThroughInputNb;
        TimeValue passThroughTime;
        ViewIdx passThroughView;
        bool doR, doG, doB, doA;
        bool processAllLayers;
    };

    ShmData _data;

};

inline GetComponentsResultsPtr
toGetComponentsResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetComponentsResults>(entry);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_EFFECTINSTANCEACTIONRESULTS_H

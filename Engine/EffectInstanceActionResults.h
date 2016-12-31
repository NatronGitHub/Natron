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

#include <bitset>
#include <map>
#include <list>
#include <vector>

#include "Engine/EngineFwd.h"

#include "Global/GlobalDefines.h"

#include "Engine/CacheEntryBase.h"
#include "Engine/Distorsion2D.h"
#include "Engine/ImageComponents.h"
#include "Engine/RectD.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;


class EffectInstanceActionKeyBase : public CacheEntryKeyBase
{
public:

    EffectInstanceActionKeyBase(U64 nodeFrameViewHash,
                             const RenderScale& scale,
                             const std::string& pluginID);

    virtual ~EffectInstanceActionKeyBase()
    {
        
    }

    virtual void copy(const CacheEntryKeyBase& other) OVERRIDE FINAL;

    virtual bool equals(const CacheEntryKeyBase& other) OVERRIDE FINAL;

private:

    virtual void appendToHash(Hash64* hash) const OVERRIDE FINAL;

    U64 _nodeFrameViewHash;
    RenderScale _scale;
};

class GetRegionOfDefinitionKey : public EffectInstanceActionKeyBase
{
public:

    GetRegionOfDefinitionKey(U64 nodeFrameViewHash,
                             const RenderScale& scale,
                             const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, scale, pluginID)
    {

    }

    virtual ~GetRegionOfDefinitionKey()
    {

    }

private:

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
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    const RectD& getRoD() const;
    void setRoD(const RectD& rod);

    virtual std::size_t getSize() const OVERRIDE FINAL;

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

    IsIdentityKey(U64 nodeFrameViewHash,
                  const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, RenderScale(1.), pluginID)
    {

    }

    virtual ~IsIdentityKey()
    {

    }

private:

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
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    void getIdentityData(int* identityInputNb, TimeValue* identityTime, ViewIdx* identityView) const;
    void setIdentityData(int identityInputNb, TimeValue identityTime, ViewIdx identityView);

    virtual std::size_t getSize() const OVERRIDE FINAL;

private:

    int _identityInputNb;
    TimeValue _identityTime;
    ViewIdx _identityView;

};

inline IsIdentityResultsPtr
toIsIdentityResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<IsIdentityResults>(entry);
}



class GetFramesNeededKey : public EffectInstanceActionKeyBase
{
public:

    GetFramesNeededKey(U64 nodeFrameViewHash,
                  const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, RenderScale(1.), pluginID)
    {

    }

    virtual ~GetFramesNeededKey()
    {

    }

private:

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
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    void getFramesNeeded(FramesNeededMap* framesNeeded) const;
    void setFramesNeeded(const FramesNeededMap& framesNeeded);

    virtual std::size_t getSize() const OVERRIDE FINAL;

private:

    FramesNeededMap _framesNeeded;

};

inline GetFramesNeededResultsPtr
toGetFramesNeededResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetFramesNeededResults>(entry);
}


class GetDistorsionKey : public EffectInstanceActionKeyBase
{
public:

    GetDistorsionKey(U64 nodeFrameViewHash,
                       const RenderScale& scale,
                       const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, scale, pluginID)
    {

    }

    virtual ~GetDistorsionKey()
    {

    }

private:

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetDistorsionResults;
    }
    
};

class GetDistorsionResults : public CacheEntryBase
{
    GetDistorsionResults();

public:

    static GetDistorsionResultsPtr create(const GetDistorsionKeyPtr& key);

    virtual ~GetDistorsionResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    DistorsionFunction2DPtr getDistorsionResults() const;
    void setDistorsionResults(const DistorsionFunction2DPtr& disto);

    virtual std::size_t getSize() const OVERRIDE FINAL;

private:

    DistorsionFunction2DPtr _distorsion;

};

inline GetDistorsionResultsPtr
toGetDistorsionResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetDistorsionResults>(entry);
}


class GetFrameRangeKey : public EffectInstanceActionKeyBase
{
public:

    GetFrameRangeKey(U64 nodeTimeViewInvariantHash,
                     const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewInvariantHash, RenderScale(1.), pluginID)
    {

    }

    virtual ~GetFrameRangeKey()
    {

    }

private:

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
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    void getFrameRangeResults(RangeD* range) const;
    void setFrameRangeResults(const RangeD& range);

    virtual std::size_t getSize() const OVERRIDE FINAL;

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
    : EffectInstanceActionKeyBase(nodeTimeViewInvariantHash, RenderScale(1.), pluginID)
    {

    }

    virtual ~GetTimeInvariantMetaDatasKey()
    {

    }

private:

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
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    const NodeMetadataPtr& getMetadatasResults() const;
    void setMetadatasResults(const NodeMetadataPtr& metadatas);

    virtual std::size_t getSize() const OVERRIDE FINAL;

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

    GetComponentsKey(U64 nodeFrameViewHash,
                     const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, RenderScale(1.), pluginID)
    {

    }

    virtual ~GetComponentsKey()
    {

    }

private:

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
    // To ensure this you may call
    // assert(getCacheBucketIndex() == -1) in any setter function.
    void getResults(std::map<int, std::list<ImageComponents> >* neededInputLayers,
                    std::list<ImageComponents>* producedLayers,
                    std::list<ImageComponents>* passThroughPlanes,
                    int *passThroughInputNb,
                    TimeValue *passThroughTime,
                    ViewIdx *passThroughView,
                    std::bitset<4> *processChannels,
                    bool *processAllLayers) const;

    void setResults(const std::map<int, std::list<ImageComponents> >& neededInputLayers,
                    const std::list<ImageComponents>& producedLayers,
                    const std::list<ImageComponents>& passThroughPlanes,
                    int passThroughInputNb,
                    TimeValue passThroughTime,
                    ViewIdx passThroughView,
                    std::bitset<4> processChannels,
                    bool processAllLayers);


    virtual std::size_t getSize() const OVERRIDE FINAL;

private:

    std::map<int, std::list<ImageComponents> > _neededInputLayers;
    std::list<ImageComponents> _producedLayers;
    std::list<ImageComponents> _passThroughPlanes;
    int _passThroughInputNb;
    TimeValue _passThroughTime;
    ViewIdx _passThroughView;
    std::bitset<4> _processChannels;
    bool _processAllLayers;


};

inline GetComponentsResultsPtr
toGetComponentsResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetComponentsResults>(entry);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_EFFECTINSTANCEACTIONRESULTS_H

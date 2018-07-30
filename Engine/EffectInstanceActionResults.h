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

NATRON_NAMESPACE_ENTER


class EffectInstanceActionKeyBase : public CacheEntryKeyBase
{
public:

    EffectInstanceActionKeyBase(U64 nodeTimeViewVariantHash,
                                const RenderScale& scale,
                                const std::string& pluginID);

    virtual ~EffectInstanceActionKeyBase()
    {
        
    }



    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE;

    virtual CacheEntryKeyBase::FromMemorySegmentRetCodeEnum fromMemorySegment(const IPCPropertyMap& properties) OVERRIDE;


protected:

    virtual void appendToHash(Hash64* hash) const OVERRIDE;

private:

    struct KeyShmData
    {
        U64 nodeTimeViewVariantHash;
        RenderScale scale;
    };

    KeyShmData _data;
};

class GetRegionOfDefinitionKey : public EffectInstanceActionKeyBase
{
public:

    GetRegionOfDefinitionKey(U64 nodeTimeViewVariantHash,
                             const RenderScale& scale,
                             const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewVariantHash, scale, pluginID)
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

    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;


private:

    RectD _rod;

};

inline GetRegionOfDefinitionResultsPtr
toGetRegionOfDefinitionResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetRegionOfDefinitionResults>(entry);
}

class GetDistortionKey : public EffectInstanceActionKeyBase
{
public:

    GetDistortionKey(U64 nodeTimeViewVariantHash,
                      const RenderScale& scale,
                      const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewVariantHash, scale, pluginID)
    {

    }

    virtual ~GetDistortionKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetDistortionResults;
    }



};

/**
 * @brief Beware: this is the only action result that may NOT live in a persistent storage because the plug-in returns to us 
 * a pointer to memory it holds.
 **/
class GetDistortionResults : public CacheEntryBase
{
    GetDistortionResults();

public:

    static GetDistortionResultsPtr create(const GetDistortionKeyPtr& key);

    virtual ~GetDistortionResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    DistortionFunction2DPtr getResults() const;
    void setResults(const DistortionFunction2DPtr& results);

    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;
    
    
private:
    
    DistortionFunction2DPtr _disto;
    
};


inline GetDistortionResultsPtr
toGetDistortionResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetDistortionResults>(entry);
}

class IsIdentityKey : public EffectInstanceActionKeyBase
{
public:

    IsIdentityKey(U64 nodeTimeViewVariantHash,
                  TimeValue time,
                  const ImagePlaneDesc& plane,
                  const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewVariantHash, RenderScale(1.), pluginID)
    , _time(time)
    , _plane(plane)
    {

    }

    virtual ~IsIdentityKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDIsIdentityResults;
    }

private:

    virtual void appendToHash(Hash64* hash) const OVERRIDE FINAL;

    // If an effect is not frame varying, it will not encode the time in the hash, however the results of isIdentity
    // may change across time even for non frame varying effects: e.g: A single frame Reader would return identity = -2
    // for any time different than frame 1 and -1 otherwise.
    TimeValue _time;
    ImagePlaneDesc _plane;
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
    void getIdentityData(int* identityInputNb, TimeValue* identityTime, ViewIdx* identityView, ImagePlaneDesc *identityPlane) const;
    void setIdentityData(int identityInputNb, TimeValue identityTime, ViewIdx identityView, const ImagePlaneDesc &identityPlane);

    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;

private:


    struct ShmData
    {
        int identityInputNb;
        TimeValue identityTime;
        ViewIdx identityView;
        ImagePlaneDesc identityPlane;
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

    GetFramesNeededKey(U64 nodeTimeViewVariantHash,
                       const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewVariantHash, RenderScale(1.), pluginID)
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

    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;

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
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash, RenderScale(1.), pluginID)
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

    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;

private:

    RangeD _range;

};

inline GetFrameRangeResultsPtr
toGetFrameRangeResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetFrameRangeResults>(entry);
}


class GetTimeInvariantMetadataKey : public EffectInstanceActionKeyBase
{
public:

    GetTimeInvariantMetadataKey(U64 nodeTimeInvariantHash,
                                 const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeInvariantHash,  RenderScale(1.), pluginID)
    {

    }

    virtual ~GetTimeInvariantMetadataKey()
    {

    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDGetTimeInvariantMetadataResults;
    }
};


class GetTimeInvariantMetadataResults : public CacheEntryBase
{
    GetTimeInvariantMetadataResults();

public:

    static GetTimeInvariantMetadataResultsPtr create(const GetTimeInvariantMetadataKeyPtr& key);

    virtual ~GetTimeInvariantMetadataResults()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    const NodeMetadataPtr& getMetadataResults() const;
    void setMetadataResults(const NodeMetadataPtr& metadata);

    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;

private:

    NodeMetadataPtr _metadata;

};

inline GetTimeInvariantMetadataResultsPtr
toGetTimeInvariantMetadataResults(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GetTimeInvariantMetadataResults>(entry);
}


class GetComponentsKey : public EffectInstanceActionKeyBase
{
public:

    GetComponentsKey(U64 nodeTimeViewVariantHash,
                     const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeTimeViewVariantHash, RenderScale(1.), pluginID)
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

    const std::map<int, std::list<ImagePlaneDesc> >& getNeededInputPlanes() const;

    std::bitset<4> getProcessChannels() const;

    const std::list<ImagePlaneDesc>& getProducedPlanes() const;

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


    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;
    
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

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_EFFECTINSTANCEACTIONRESULTS_H

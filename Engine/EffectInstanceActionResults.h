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

#include "Engine/EngineFwd.h"

#include "Global/GlobalDefines.h"

#include "Engine/CacheEntryBase.h"
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
                             const RenderScale& scale,
                             const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, scale, pluginID)
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
                  const RenderScale& scale,
                  const std::string& pluginID)
    : EffectInstanceActionKeyBase(nodeFrameViewHash, scale, pluginID)
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
NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_EFFECTINSTANCEACTIONRESULTS_H

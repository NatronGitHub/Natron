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

#ifndef NATRON_ENGINE_CACHEENTRYBASE_H
#define NATRON_ENGINE_CACHEENTRYBASE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"
#include "Serialization/SerializationBase.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief The base class for a key of an item in the cache
 **/
struct CacheEntryKeyBasePrivate;
class CacheEntryKeyBase : public boost::noncopyable
{
public:


    CacheEntryKeyBase();
    
    CacheEntryKeyBase(const std::string& pluginID);

    virtual ~CacheEntryKeyBase();

    /**
     * @brief Override to copy the other key. Make sure to call
     * the base-class version aswell.
     **/
    virtual void copy(const CacheEntryKeyBase& other);

    /**
     * @brief Override to check if this key equals the other key.
     * This should not compare the hash as the purpose of this function
     * is to validate that the key matches.
     **/
    virtual bool equals(const CacheEntryKeyBase& other) = 0;

    /**
     * @brief Get the hash for this key.
     **/
    U64 getHash() const;

    /**
     * @brief Return the plug-in ID to which this cache entry corresponds to.
     **/
    std::string getHolderPluginID() const;
    void setHolderPluginID(const std::string& holderID);
    
protected:

    /**
     * @brief Must append anything that should identify uniquely the cache entry.
     **/
    virtual void appendToHash(Hash64* hash) const = 0;


private:

    boost::scoped_ptr<CacheEntryKeyBasePrivate> _imp;
   
};

struct ImageTileKeyPrivate;
class ImageTileKey
: public CacheEntryKeyBase
, public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
public:

    ImageTileKey();

    ImageTileKey(U64 nodeFrameViewHashKey,
                 const std::string& layerChannel,
                 unsigned int mipMapLevel,
                 bool draftMode,
                 ImageBitDepthEnum bitdepth,
                 int tileX,
                 int tileY);

    virtual ~ImageTileKey();

    U64 getNodeFrameViewHashKey() const;

    std::string getLayerChannel() const;

    int getTileX() const;

    int getTileY() const;

    unsigned int getMipMapLevel() const;

    bool isDraftMode() const;

    ImageBitDepthEnum getBitDepth() const;

    virtual bool equals(const CacheEntryKeyBase& other) OVERRIDE FINAL;

    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

private:

    virtual void appendToHash(Hash64* hash) const OVERRIDE FINAL;

    boost::scoped_ptr<ImageTileKeyPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_CACHEENTRYBASE_H

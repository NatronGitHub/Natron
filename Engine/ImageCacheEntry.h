/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_IMAGECACHEENTRY_H
#define NATRON_ENGINE_IMAGECACHEENTRY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/ImageCacheKey.h"
#include "Engine/ImageTilesState.h"
#include "Engine/CacheEntryBase.h"

NATRON_NAMESPACE_ENTER;

struct ImageCacheEntryPrivate;

/**
 * @brief Interface between an image and the cache.
 * This is the object that lives in the cache.
 * A storage is passed in parameter that is assumed to of the size of the
 * roi and the cache data is copied to this storage.
 **/
class ImageCacheEntry : public CacheEntryBase
{
public:

    ImageCacheEntry(const RectI& pixelRod,
                    const RectI& roi,
                    ImageBitDepthEnum depth,
                    int nComps,
                    const ImageStorageBasePtr& storage,
                    const EffectInstancePtr& effect);

    virtual ~ImageCacheEntry();

    virtual void toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment,
                                   ExternalSegmentTypeHandleList::const_iterator start,
                                   ExternalSegmentTypeHandleList::const_iterator end) OVERRIDE FINAL;


    /**
     * @brief We allow a single thread to fetch this image multiple times without computing necessarily on the first fetch.
     * Without this the cache would hang.
     **/
    virtual bool allowMultipleFetchForThread() const OVERRIDE FINAL
    {
        return true;
    }

    virtual std::size_t getNumTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN;



private:

    boost::scoped_ptr<ImageCacheEntryPrivate> _imp;
};
NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGECACHEENTRY_H

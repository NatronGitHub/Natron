/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NONKEYPARAMS_H
#define NONKEYPARAMS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstddef>

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"
#include "Engine/RectI.h"


NATRON_NAMESPACE_ENTER

inline int
getSizeOfForBitDepth(ImageBitDepthEnum bitdepth)
{
    switch (bitdepth) {
    case eImageBitDepthByte: {
        return sizeof(unsigned char);
    }
    case eImageBitDepthShort: {
        return sizeof(unsigned short);
    }
    case eImageBitDepthHalf: {
        return sizeof(unsigned short);
    }
    case eImageBitDepthFloat: {
        return sizeof(float);
    }
    case eImageBitDepthNone:
        break;
    }

    return 0;
}

struct CacheEntryStorageInfo
{
    RectI bounds;
    std::size_t dataTypeSize;
    std::size_t numComponents;
    StorageModeEnum mode;
    U32 textureTarget;

    CacheEntryStorageInfo()
        : bounds()
        , dataTypeSize(0)
        , numComponents(0)
        , mode(eStorageModeNone)
        , textureTarget(0)
    {
    }

    bool operator==(const CacheEntryStorageInfo& other) const
    {
        return dataTypeSize == other.dataTypeSize &&
               mode == other.mode &&
               textureTarget == other.textureTarget;
    }
};

class NonKeyParams
{
public:

    NonKeyParams();

    virtual ~NonKeyParams()
    {
    }

    NonKeyParams(const CacheEntryStorageInfo& info);

    NonKeyParams(const NonKeyParams & other);

    const CacheEntryStorageInfo& getStorageInfo() const;
    CacheEntryStorageInfo& getStorageInfo();

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/);

protected:

    bool operator==(const NonKeyParams & other) const
    {
        return (_storageInfo == other._storageInfo);
    }

    bool operator!=(const NonKeyParams & other) const
    {
        return !(*this == other);
    }

private:

    CacheEntryStorageInfo _storageInfo;
};

NATRON_NAMESPACE_EXIT

#endif // NONKEYPARAMS_H

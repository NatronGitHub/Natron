/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef NATRON_ENGINE_RAMBUFFER_H
#define NATRON_ENGINE_RAMBUFFER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstdlib>
#include <stdexcept>

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

template <typename T>
class RamBuffer
{
    T* data;
    U64 count;

public:

    RamBuffer()
    : data(0)
    , count(0)
    {
    }

    ~RamBuffer()
    {
        clear();
    }

    T* getData()
    {
        return data;
    }

    const T* getData() const
    {
        return data;
    }

    void swap(RamBuffer& other)
    {
        std::swap(data, other.data);
        std::swap(count, other.count);
    }

    U64 size() const
    {
        return count;
    }

    void resize(U64 size)
    {
        if (size == 0) {
            return;
        }
        count = size;
        if (data) {
            free(data);
            data = 0;
        }
        if (count == 0) {
            return;
        }
        data = (T*)malloc( size * sizeof(T) );
        if (!data) {
            throw std::bad_alloc();
        }
    }

    void resizeAndPreserve(U64 size)
    {
        if (size == 0 || size == count) {
            return;
        }
        count = size;
        data = (T*)realloc(data,size * sizeof(T));
        if (!data) {
            throw std::bad_alloc();
        }
    }

    void clear()
    {
        count = 0;
        if (data) {
            free(data);
            data = 0;
        }
    }


};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_RAMBUFFER_H

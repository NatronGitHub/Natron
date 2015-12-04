/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_TEXTURE_H
#define NATRON_GUI_TEXTURE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/TextureRect.h"
#include "Engine/EngineFwd.h"


class Texture
{
public:
    /*Note that the short datatype is not used currently*/
    enum DataTypeEnum
    {
        eDataTypeNone,
        eDataTypeByte,
        eDataTypeFloat,
        eDataTypeHalf,
    };

public:


    Texture(U32 target,
            int minFilter,
            int magFilter,
            int clamp);

    U32 getTexID() const
    {
        return _texID;
    }

    int w() const
    {
        return _textureRect.w;
    }

    int h() const
    {
        return _textureRect.h;
    }

    DataTypeEnum type() const
    {
        return _type;
    }
    
    bool mustAllocTexture(const TextureRect& rect) const;

    /*allocates the texture*/
    void fillOrAllocateTexture(const TextureRect & texRect, DataTypeEnum type, const RectI& roi, bool updateOnlyRoi);

    const TextureRect & getTextureRect() const
    {
        return _textureRect;
    }

    virtual ~Texture();

private:

    U32 _texID;
    U32 _target;
    int _minFilter,_magFilter, _clamp;
    TextureRect _textureRect;
    DataTypeEnum _type;
};


#endif /* defined(NATRON_GUI_TEXTURE_H_) */

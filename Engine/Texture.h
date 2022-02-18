/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER

class Texture
{
public:

    enum DataTypeEnum
    {
        eDataTypeNone,
        eDataTypeByte,
        eDataTypeFloat,
        eDataTypeUShort,
        eDataTypeHalf,
    };


    Texture(U32 target,
            int minFilter,
            int magFilter,
            int clamp,
            DataTypeEnum type,
            int format,
            int internalFormat,
            int glType);
    static void getRecommendedTexParametersForRGBAByteTexture(int* format, int* internalFormat, int* glType);
    static void getRecommendedTexParametersForRGBAFloatTexture(int* format, int* internalFormat, int* glType);

    U32 getTexID() const
    {
        return _texID;
    }

    int getTexTarget() const
    {
        return _target;
    }

    int w() const
    {
        return _textureRect.width();
    }

    int h() const
    {
        return _textureRect.height();
    }

    /**
     * @brief The bitdepth of the texture
     **/
    DataTypeEnum type() const
    {
        return _type;
    }

    std::size_t getDataSizeOf() const
    {
        switch (_type) {
        case eDataTypeByte:

            return sizeof(unsigned char);
        case eDataTypeFloat:

            return sizeof(float);
        case eDataTypeHalf:

            // Fixme when we support half
            return sizeof(float);
        case eDataTypeNone:
        default:

            return 0;
        }
    }

    /**
     * @brief Returns the size occupied by this texture in bytes
     **/
    std::size_t getSize() const
    {
        // textures are always RGBA for now.
        return _textureRect.area() * getDataSizeOf() * 4;
    }

    /*
     * @brief Ensures that the texture is of size texRect and of the given type
     * @returns True if something changed, false otherwise
     * Note: Internally this function calls glTexImage2D to reallocate the texture buffer
     * @param originalRAMBuffer Optional pointer to a mapped PBO for asynchronous texture upload
     */
    bool ensureTextureHasSize(const TextureRect& texRect, const unsigned char* originalRAMBuffer);

    /**
     * @brief Update the texture with the currently bound PBO across the given rectangle.
     * @param texRect The bounds of the texture, if the texture does not match these bounds, it will be reallocated
     * using ensureTextureHasSize(texRect,type)/
     * @param roi if updateOnlyRoi is true, this will be the portion of the texture to update with glTexSubImage2D
     * @param updateOnlyRoI if updateOnlyRoi is true, only the portion defined by roi will be updated on the texture
     **/
    void fillOrAllocateTexture(const TextureRect & texRect, const RectI& roi, bool updateOnlyRoi, const unsigned char* originalRAMBuffer);

    /**
     * @brief The bounds of the texture
     **/
    const TextureRect & getTextureRect() const
    {
        return _textureRect;
    }

    int getFormat() const
    {
        return _format;
    }

    int getInternalFormat() const
    {
        return _internalFormat;
    }

    int getGLType() const
    {
        return _glType;
    }

    virtual ~Texture();

private:

    U32 _texID;
    U32 _target;
    int _minFilter, _magFilter, _clamp;
    int _internalFormat, _format, _glType;
    TextureRect _textureRect;
    DataTypeEnum _type;
};

NATRON_NAMESPACE_EXIT

#endif /* defined(NATRON_GUI_TEXTURE_H_) */

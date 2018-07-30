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

#ifndef NATRON_GUI_TEXTURE_H
#define NATRON_GUI_TEXTURE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Global/GlobalDefines.h"
#include "Engine/RectI.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class Texture
{
public:

    Texture(U32 target,
            int minFilter,
            int magFilter,
            int clamp,
            ImageBitDepthEnum type,
            int format,
            int internalFormat,
            int glType,
            bool useOpenGL);

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
        return _bounds.width();
    }

    int h() const
    {
        return _bounds.height();
    }

    /**
     * @brief The bitdepth of the texture
     **/
    ImageBitDepthEnum getBitDepth() const
    {
        return _bitDepth;
    }

    std::size_t getDataSizeOf() const
    {
        switch (_bitDepth) {
        case eImageBitDepthByte:

            return sizeof(unsigned char);
        case eImageBitDepthFloat:

            return sizeof(float);
        case eImageBitDepthHalf:

            // Fixme when we support half
            return sizeof(float);
        case eImageBitDepthNone:
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
        return _bounds.area() * getDataSizeOf() * 4;
    }

    /*
     * @brief Ensures that the texture is of size texRect and of the given type
     * @returns True if something changed, false otherwise
     * Note: Internally this function calls glTexImage2D to reallocate the texture buffer
     * @param originalRAMBuffer Optional pointer to a mapped PBO for asynchronous texture upload
     */
    bool ensureTextureHasSize(const RectI & bounds, const unsigned char* originalRAMBuffer);

    /**
     * @brief Update the texture with the currently bound PBO across the given rectangle.
     * @param bounds The bounds of the texture, if the texture does not match these bounds, it will be reallocated
     * using ensureTextureHasSize(texRect,type)/
     * @param roi if set, this will be the portion of the texture to update with glTexSubImage2D
     **/
    void fillOrAllocateTexture(const RectI & bounds, const RectI* roiParam, const unsigned char* originalRAMBuffer);

    /**
     * @brief The bounds of the texture
     **/
    const RectI & getBounds() const
    {
        return _bounds;
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
    RectI _bounds;
    ImageBitDepthEnum _bitDepth;
    bool _useOpenGL;
};

NATRON_NAMESPACE_EXIT

#endif /* defined(NATRON_GUI_TEXTURE_H_) */

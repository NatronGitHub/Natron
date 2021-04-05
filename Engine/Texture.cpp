/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Texture.h"

#include <stdexcept>

NATRON_NAMESPACE_ENTER

Texture::Texture(U32 target,
                 int minFilter,
                 int magFilter,
                 int clamp,
                 DataTypeEnum type,
                 int format,
                 int internalFormat,
                 int glType)
    : _texID(0)
    , _target(target)
    , _minFilter(minFilter)
    , _magFilter(magFilter)
    , _clamp(clamp)
    , _internalFormat(internalFormat)
    , _format(format)
    , _glType(glType)
    , _type(type)
{
    glGenTextures(1, &_texID);
}

void
Texture::getRecommendedTexParametersForRGBAByteTexture(int* format,
                                                       int* internalFormat,
                                                       int* glType)
{
    *format = GL_BGRA;
    *internalFormat = GL_RGBA8;
    *glType = GL_UNSIGNED_INT_8_8_8_8_REV;
}

void
Texture::getRecommendedTexParametersForRGBAFloatTexture(int* format, int* internalFormat, int* glType)
{
    *format = GL_RGBA;
    *internalFormat = GL_RGBA32F_ARB;
    *glType = GL_FLOAT;
}

bool
Texture::ensureTextureHasSize(const TextureRect& texRect,
                              const unsigned char* originalRAMBuffer)
{
    if (texRect == _textureRect) {
        return false;
    }

    GLProtectAttrib a(GL_ENABLE_BIT);
    glEnable(_target);
    glBindTexture (_target, _texID);
    _textureRect = texRect;

    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    if (_minFilter != GL_NONE) {
        glTexParameteri (_target, GL_TEXTURE_MIN_FILTER, _minFilter);
    }
    if (_magFilter != GL_NONE) {
        glTexParameteri (_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    }

    if (_clamp != GL_NONE) {
        glTexParameteri (_target, GL_TEXTURE_WRAP_S, _clamp);
        glTexParameteri (_target, GL_TEXTURE_WRAP_T, _clamp);
    }

    glTexImage2D (_target,
                  0,            // level
                  _internalFormat, //internalFormat
                  w(), h(),
                  0,            // border
                  _format,      // format
                  _glType, // type
                  originalRAMBuffer);           // pixels


    glBindTexture(_target, 0);
    glCheckError();

    return true;
}

void
Texture::fillOrAllocateTexture(const TextureRect & texRect,
                               const RectI& roi,
                               bool updateOnlyRoi,
                               const unsigned char* originalRAMBuffer)
{
    //GLuint savedTexture;
    //glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib a(GL_ENABLE_BIT);
        int width = updateOnlyRoi ? roi.width() : w();
        int height = updateOnlyRoi ? roi.height() : h();
        int x1 = updateOnlyRoi ? roi.x1 - texRect.x1 : 0;
        int y1 = updateOnlyRoi ? roi.y1 - texRect.y1 : 0;

        if ( !ensureTextureHasSize(texRect, originalRAMBuffer) ) {
            glEnable(_target);
            glBindTexture (_target, _texID);

            glTexSubImage2D(_target,
                            0,              // level
                            x1, y1,               // xoffset, yoffset
                            width, height,
                            _format,            // format
                            _glType,       // type
                            originalRAMBuffer);

            glBindTexture (_target, 0);
            glCheckError();
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT);
} // fillOrAllocateTexture

Texture::~Texture()
{
    glDeleteTextures(1, &_texID);
}

NATRON_NAMESPACE_EXIT

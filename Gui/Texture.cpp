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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Texture.h"

#include <iostream>
#include "Global/GLIncludes.h"
#include "Gui/ViewerGL.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

NATRON_NAMESPACE_ENTER;

Texture::Texture(U32 target,
                 int minFilter,
                 int magFilter,
                 int clamp)
    : _target(target)
      , _minFilter(minFilter)
      , _magFilter(magFilter)
      , _clamp(clamp)
      , _type(eDataTypeNone)
{
    glGenTextures(1, &_texID);
}

bool
Texture::mustAllocTexture(const TextureRect& rect) const
{
    return _textureRect != rect;
}

void
Texture::fillOrAllocateTexture(const TextureRect & texRect,
                               DataTypeEnum type,
                               const RectI& roi,
                               bool updateOnlyRoi)
{
    //GLuint savedTexture;
    //glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib a(GL_ENABLE_BIT);

        int width = updateOnlyRoi ? roi.width() : w();
        int height = updateOnlyRoi ? roi.height() : h();
        int x1 = updateOnlyRoi ? roi.x1 - texRect.x1 : 0;
        int y1 = updateOnlyRoi ? roi.y1 - texRect.y1 : 0;
        
        glEnable(_target);
        glBindTexture (_target, _texID);
        if ( (texRect == _textureRect) && (_type == type) ) {
            if (_type == Texture::eDataTypeByte) {
                glTexSubImage2D(_target,
                                0,              // level
                                x1, y1,               // xoffset, yoffset
                                width, height,
                                GL_BGRA,            // format
                                GL_UNSIGNED_INT_8_8_8_8_REV,        // type
                                0);
            } else if (_type == Texture::eDataTypeFloat) {
                glTexSubImage2D(_target,
                                0,              // level
                                x1, y1,               // xoffset, yoffset
                                width, height,
                                GL_RGBA,            // format
                                GL_FLOAT,       // type
                                0);
            }
            glCheckError();
        } else {
            _textureRect = texRect;
            _type = type;
            glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

            glTexParameteri (_target, GL_TEXTURE_MIN_FILTER, _minFilter);
            glTexParameteri (_target, GL_TEXTURE_MAG_FILTER, _magFilter);

            glTexParameteri (_target, GL_TEXTURE_WRAP_S, _clamp);
            glTexParameteri (_target, GL_TEXTURE_WRAP_T, _clamp);
            if (type == eDataTypeByte) {
                glTexImage2D(_target,
                             0,         // level
                             GL_RGBA8, //internalFormat
                             w(), h(),
                             0,         // border
                             GL_BGRA,       // format
                             GL_UNSIGNED_INT_8_8_8_8_REV,   // type
                             0);            // pixels
            } else if (type == eDataTypeFloat) {
                glTexImage2D (_target,
                              0,            // level
                              GL_RGBA32F_ARB, //internalFormat
                              w(), h(),
                              0,            // border
                              GL_RGBA,      // format
                              GL_FLOAT, // type
                              0);           // pixels
            }
            
            glCheckError();
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT);
} // fillOrAllocateTexture

Texture::~Texture()
{
    glDeleteTextures(1, &_texID);
}

NATRON_NAMESPACE_EXIT;

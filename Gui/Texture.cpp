//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Texture.h"

#include <iostream>
#include "Global/GLIncludes.h"
#include "Gui/ViewerGL.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

Texture::Texture(U32 target,
                 int minFilter,
                 int magFilter)
    : _target(target)
      , _minFilter(minFilter)
      , _magFilter(magFilter)
{
    glGenTextures(1, &_texID);
}

void
Texture::fillOrAllocateTexture(const TextureRect & texRect,
                               DataType type)
{
    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    glPushAttrib(GL_ENABLE_BIT);
    {
        glEnable(_target);
        glBindTexture (_target, _texID);
        if ( (texRect == _textureRect) && (_type == type) ) {
            if (_type == Texture::BYTE) {
                glTexSubImage2D(_target,
                                0,              // level
                                0, 0,               // xoffset, yoffset
                                w(), h(),
                                GL_BGRA,            // format
                                GL_UNSIGNED_INT_8_8_8_8_REV,        // type
                                0);
            } else if (_type == Texture::FLOAT) {
                glTexSubImage2D(_target,
                                0,              // level
                                0, 0,               // xoffset, yoffset
                                w(), h(),
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

            glTexParameteri (_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri (_target, GL_TEXTURE_WRAP_T, GL_CLAMP);
            if (type == BYTE) {
                glTexImage2D(_target,
                             0,         // level
                             GL_RGBA8, //internalFormat
                             w(), h(),
                             0,         // border
                             GL_BGRA,       // format
                             GL_UNSIGNED_INT_8_8_8_8_REV,   // type
                             0);            // pixels
            } else if (type == FLOAT) {
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
    } // glPushAttrib(GL_ENABLE_BIT);
    glPopAttrib();
} // fillOrAllocateTexture

Texture::~Texture()
{
    glDeleteTextures(1, &_texID);
}


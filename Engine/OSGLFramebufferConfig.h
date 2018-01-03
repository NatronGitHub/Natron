/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef OSGLFRAMEBUFFERCONFIG_H
#define OSGLFRAMEBUFFERCONFIG_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/* @brief Framebuffer configuration.
 *
 *  This describes buffers and their sizes.  It also contains
 *  a platform-specific ID used to map back to the backend API object.
 *
 *  It is used to pass framebuffer parameters from shared code to the platform
 *  API and also to enumerate and select available framebuffer configs.
 */
class FramebufferConfig
{

public:

    static const int ATTR_DONT_CARE = -1;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int depthBits;
    int stencilBits;
    int accumRedBits;
    int accumGreenBits;
    int accumBlueBits;
    int accumAlphaBits;
    int auxBuffers;
    unsigned char stereo;
    int samples;
    unsigned char sRGB;
    unsigned char doublebuffer;
    unsigned long handle;

    FramebufferConfig()
    : redBits(8)
    , greenBits(8)
    , blueBits(8)
    , alphaBits(8)
    , depthBits(24)
    , stencilBits(8)
    , accumRedBits(0)
    , accumGreenBits(0)
    , accumBlueBits(0)
    , accumAlphaBits(0)
    , auxBuffers(0)
    , stereo(0)
    , samples(0)
    , sRGB(0)
    , doublebuffer(0)
    , handle(0)
    {

    }
};



class GLRendererID
{

public:

    int renderID;

    // wgl NV extension use handles and not integers
    void* rendererHandle;

    GLRendererID()
    : renderID(-1)
    , rendererHandle(0) {}

    explicit GLRendererID(int id) : renderID(id), rendererHandle(0) {}

    explicit GLRendererID(void* handle) : renderID(0), rendererHandle(handle) {}
};

class OpenGLRendererInfo
{
public:

    std::size_t maxMemBytes;
    std::string rendererName;
    std::string vendorName;
    std::string glVersionString;
    std::string glslVersionString;
    int maxTextureSize;
    GLRendererID rendererID;

    OpenGLRendererInfo()
    : maxMemBytes(0)
    , rendererName()
    , vendorName()
    , glVersionString()
    , maxTextureSize(0)
    , rendererID(0)
    {

    }
};

NATRON_NAMESPACE_EXIT
#endif // OSGLFRAMEBUFFERCONFIG_H

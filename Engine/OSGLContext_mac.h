/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef OSGLCONTEXT_MAC_H
#define OSGLCONTEXT_MAC_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

#ifdef __NATRON_OSX__

NATRON_NAMESPACE_ENTER

class OSGLContext_mac
{
public:

    OSGLContext_mac(const FramebufferConfig& pixelFormatAttrs,
                    int major,
                    int minor,
                    bool coreProfile,
                    const GLRendererID& rendererID,
                    const OSGLContext_mac* shareContext);

    ~OSGLContext_mac();

    static void makeContextCurrent(const OSGLContext_mac* context);
    static bool threadHasACurrentContext();
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

    void swapBuffers();

    void swapInterval(int interval);

private:
    class Implementation;
    std::unique_ptr<Implementation> _imp; // PIMPL: hide implementation details
};

NATRON_NAMESPACE_EXIT

#endif // __NATRON_OSX__

#endif // OSGLCONTEXT_MAC_H

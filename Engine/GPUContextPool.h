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


#ifndef GPUCONTEXTPOOL_H
#define GPUCONTEXTPOOL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

// If set, multiple frame renders may use the same GPU context instead of locking it for the whole render of a frame.
#define NATRON_RENDER_SHARED_CONTEXT

struct GPUContextPoolPrivate;
class GPUContextPool
{
public:

    GPUContextPool();

    ~GPUContextPool();


    //////////////////////////////// OpenGL related /////////////////////////////////

    /**
     * @brief Attaches one of the OpenGL context in the pool to a specific frame render.
     * The context will not be available to other renders until releaseGLContextFromRender() is called
     * for on the same thread that called this function.
     * This function is blocking and the calling thread will wait until one context is made available
     * in the pool again.
     * If a context is already attached on the thread, this function returns immediately
     * After returning this function, the context must be made current before
     * OpenGL calls can be made.
     * If during the render, other threads are participating and needs to make the context current, they
     * may call makeContextCurrent with the context returned from this function.
     *
     * @param renderUniqueAbortInfo These are the infos that uniquely identify the render attached to the context
     **/
    OSGLContextPtr attachGLContextToRender(bool checkIfGLLoaded = true);

    /**
     * @brief Releases the given OpenGL context from a render. After this call the context may be re-used
     * by other threads waiting in attachContextToThread().
     * This function MUST be call when a render is finished and the argument passed should be the context returned
     * from the function attachGLContextToRender().
     **/
    void releaseGLContextFromRender(const OSGLContextPtr& context);

    /**
     * @brief Returns the max texture size, i.e: the value returned by glGetIntegerv(GL_MAX_TEXTURE_SIZE,&v)
     * This does not call glGetIntegerv and does not require a context to be bound.
     **/
    int getCurrentOpenGLRendererMaxTextureSize() const;

    ////////////////////////////////////////////////////////////////

    /**
     * @brief Clear all created contexts
     **/
    void clear();

private:


    std::unique_ptr<GPUContextPoolPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // GPUCONTEXTPOOL_H

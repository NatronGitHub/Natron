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


#ifndef GPUCONTEXTPOOL_H
#define GPUCONTEXTPOOL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;


struct GPUContextPoolPrivate;
class GPUContextPool
{
public:

    GPUContextPool();

    ~GPUContextPool();


    //////////////////////////////// OpenGL related /////////////////////////////////

    /**
     * @brief Attaches one of the OpenGL context in the pool to a specific frame render.
     * Multiple frame renders may share the same context, but when they lock it with the 
     * setContextCurrent() function, they own the context and lock out all other renders trying to use it.
     * After returning this function, the context must be made current before
     * OpenGL calls can be made.
     *
     * @param checkIfGLLoaded If true, this function will check if OpenGL was loaded before attempting to create a context
     **/
    OSGLContextPtr attachGLContextToRender(bool checkIfGLLoaded = true);

    /**
     * @brief Releases the given OpenGL context from a render which was previously retrieved from attachGLContextToRender()
     **/
    void releaseGLContextFromRender(const OSGLContextPtr& context);

    /**
     * @brief Returns the max texture size, i.e: the value returned by glGetIntegerv(GL_MAX_TEXTURE_SIZE,&v)
     * This does not call glGetIntegerv and does not require a context to be bound.
     **/
    int getCurrentOpenGLRendererMaxTextureSize() const;

    ////////////////////////////////////////////////////////////////

    ////////////////////////////// OpenGL CPU related (OSMesa) //////////////////////

    /**
     * @brief Attaches one of the OpenGL context in the pool to a specific frame render.
     * Multiple frame renders may share the same context, but when they lock it with the
     * setContextCurrent() function, they own the context and lock out all other renders trying to use it.
     * After returning this function, the context must be made current before
     * OpenGL calls can be made.
     *
     * @param checkIfGLLoaded If true, this function will check if OpenGL was loaded before attempting to create a context
     **/
    OSGLContextPtr attachCPUGLContextToRender();

    /**
     * @brief Releases the given OpenGL context from a render which was previously retrieved from attachGLContextToRender()
     **/
    void releaseCPUGLContextFromRender(const OSGLContextPtr& context);

    /**
     * @brief Returns the max texture size, i.e: the value returned by glGetIntegerv(GL_MAX_TEXTURE_SIZE,&v)
     * This does not call glGetIntegerv and does not require a context to be bound.
     **/
    int getCurrentCPUOpenGLRendererMaxTextureSize() const;


    ////////////////////////////////////////////////////////////////


    /**
     * @brief Clear all created contexts
     **/
    void clear();

private:


    boost::scoped_ptr<GPUContextPoolPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // GPUCONTEXTPOOL_H

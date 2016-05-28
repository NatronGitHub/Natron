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

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

// Contains all possible gpu context
struct GPUContext
{
    OSGLContextPtr glContext;
};

typedef boost::shared_ptr<GPUContext> GPUContextPtr;

struct GPUContextPoolPrivate;
class GPUContextPool
{
public:

    GPUContextPool(const int maxContextCount);

    ~GPUContextPool();

    /**
     * @brief Set the maximum number of GPU context used by the pool to be this amount
     **/
    void setMaxContextCount(int maxContextCount);

    //////////////////////////////// OpenGL related /////////////////////////////////
    /**
     * @brief Returns the number of currently created OpenGL context
     **/
    int getNumCreatedGLContext() const;

    /**
     * @brief Returns the number of attached OpenGL context. An attached context is currently
     * used by a thread
     **/
    int getNumAttachedGLContext() const;

    /**
     * @brief Attaches one of the OpenGL context in the pool to the caller thread.
     * The context will not be available to other threads until releaseGLContextFromThread() is called
     * for this thread.
     * This function is blocking and the calling thread will wait until one context is made available
     * in the pool again.
     * If a context is already attached on the thread, this function returns immediately
     * After returning this function, the context is guarenteed to be made current and 
     * OpenGL called can be made.
     **/
    void attachGLContextToThread();

    /**
     * @brief Releases the OpenGL context held by the caller thread. After this call the context may be re-used
     * by other threads waiting in attachContextToThread()
     **/
    void releaseGLContextFromThread();

    /**
     * @brief Returns the current OpenGL context for this thread.
     **/
    OSGLContextPtr getCurrentOGLContext() const;
    ////////////////////////////////////////////////////////////////

    struct ContextPoolTLSData
    {
        // One context per thread
        GPUContextPtr currentContext;
    };


private:


    boost::scoped_ptr<GPUContextPoolPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // GPUCONTEXTPOOL_H

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

NATRON_NAMESPACE_ENTER


struct GPUContextPoolPrivate;
class GPUContextPool
{
public:

    GPUContextPool();

    ~GPUContextPool();

    /**
     * @brief Get an existing OpenGL context in the GPU pool or create a new one.
     * This function cycles through existing contexts so that each contexts gets to work.
     * When exiting this function, the context is not necessarily current to the thread.
     * To make it current, create a OSGLContextAttacher object and call the attach() function.
     *
     * @param retrieveLastContext If true, the context that was asked for the last time is re-used
     * @param checkIfGLLoaded If true, this function will check if OpenGL was loaded before attempting to create a context
     **/
    OSGLContextPtr getOrCreateOpenGLContext(bool retrieveLastContext = false, bool checkIfGLLoaded = true);

    /**
     * @brief Get an existing OpenGL OSMA context in the GPU pool or create a new one.
     * This function cycles through existing contexts so that each contexts gets to work.
     * When exiting this function, the context is not necessarily current to the thread.
     * To make it current, create a OSGLContextAttacher object and call the attach() function.
     *
     * @param retrieveLastContext If true, the context that was asked for the last time is re-used
     **/
    OSGLContextPtr getOrCreateCPUOpenGLContext(bool retrieveLastContext = false);


    /**
     * @brief Clear all created contexts
     **/
    void clear();

    /**
     * @brief If this thread currently has a bound OpenGL context, this returns a valid pointer to the object
     * that attached the context, otherwise NULL.
     **/
    OSGLContextAttacherPtr getThreadLocalContext() const;

private:

    void registerContextForThread(const OSGLContextAttacherPtr& context);

    void unregisterContextForThread();


    friend class OSGLContextAttacher;
    boost::scoped_ptr<GPUContextPoolPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // GPUCONTEXTPOOL_H

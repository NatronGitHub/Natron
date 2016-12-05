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

#include "GPUContextPool.h"

#include <set>
#include <stdexcept>

#include <QMutex>
#include <QWaitCondition>

#include "Engine/AppManager.h"
#include "Engine/OSGLContext.h"
#include "Engine/Settings.h"

#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER;

struct GPUContextPoolPrivate
{
    mutable QMutex contextPoolMutex;

    // protected by contextPoolMutex
    std::set<OSGLContextPtr> glContextPool;

    OSGLContextWPtr lastUsedGLContext;

    // The OpenGL context to use for sharing
    OSGLContextWPtr glShareContext;


    std::set<OSGLContextPtr> cpuGLContextPool;
    OSGLContextWPtr lastUsedCPUGLContext;
    OSGLContextWPtr cpuGLShareContext;

    GPUContextPoolPrivate()
        : contextPoolMutex()
        , glContextPool()
        , lastUsedGLContext()
        , glShareContext()
        , cpuGLContextPool()
        , lastUsedCPUGLContext()
        , cpuGLShareContext()
    {
    }
};

GPUContextPool::GPUContextPool()
    : _imp( new GPUContextPoolPrivate() )
{
}

GPUContextPool::~GPUContextPool()
{
}

void
GPUContextPool::clear()
{
    QMutexLocker k(&_imp->contextPoolMutex);

    _imp->glContextPool.clear();
}

OSGLContextPtr
GPUContextPool::attachGLContextToRender(bool retrieveLastContext, bool checkIfGLLoaded)
{
    if (checkIfGLLoaded && (!appPTR->isOpenGLLoaded() || !appPTR->getCurrentSettings()->isOpenGLRenderingEnabled())) {
        return OSGLContextPtr();
    }
    QMutexLocker k(&_imp->contextPoolMutex);

    if (retrieveLastContext) {
        OSGLContextPtr lastCtx = _imp->lastUsedGLContext.lock();
        if (lastCtx) {
            return lastCtx;
        }
    }

    // Context-sharing disabled as it is not needed
    OSGLContextPtr shareContext;// _imp->glShareContext.lock();
    OSGLContextPtr newContext;
    SettingsPtr settings =  appPTR->getCurrentSettings();
    GLRendererID rendererID;
    if (settings) {
        rendererID = settings->getActiveOpenGLRendererID();
    }

    int maxContexts = settings ? std::max(settings->getMaxOpenGLContexts(), 1) : 1;

    if ( (int)_imp->glContextPool.size() < maxContexts ) {
        //  Create a new one
        newContext = OSGLContext::create( FramebufferConfig(), shareContext.get(), true /*useGPU*/, -1, -1, rendererID );
        _imp->glContextPool.insert(newContext);
    } else {
        while ((int)_imp->glContextPool.size() > maxContexts) {
            _imp->glContextPool.erase(_imp->glContextPool.begin());
        }

        // Cycle through all contexts for all renders
        OSGLContextPtr lastContext = _imp->lastUsedGLContext.lock();
        if (!lastContext) {
            newContext = *_imp->glContextPool.begin();
        } else {
            std::set<OSGLContextPtr>::iterator foundLast = _imp->glContextPool.find(lastContext);
            assert( foundLast != _imp->glContextPool.end() );
            if ( foundLast == _imp->glContextPool.end() ) {
                throw std::logic_error("No context to attach");
            } else {
                std::set<OSGLContextPtr>::iterator next = foundLast;
                ++next;
                if ( next == _imp->glContextPool.end() ) {
                    next = _imp->glContextPool.begin();
                }
                newContext = *next;
            }
        }
    }

    assert(newContext);

    if (settings) {
        // Initialize once static max size props
        (void)newContext->getMaxOpenGLHeight();
        (void)newContext->getMaxOpenGLWidth();
    }

    // If this is the first context, set it as the sharing context
    if (!shareContext) {
        _imp->glShareContext = newContext;
    }

    _imp->lastUsedGLContext = newContext;

    return newContext;
} // GPUContextPool::attachGLContextToRender

void
GPUContextPool::releaseGLContextFromRender(const OSGLContextPtr& context)
{
    Q_UNUSED(context);
}

OSGLContextPtr
GPUContextPool::attachCPUGLContextToRender(bool retrieveLastContext)
{
#ifdef HAVE_OSMESA
    QMutexLocker k(&_imp->contextPoolMutex);

    if (retrieveLastContext) {
        OSGLContextPtr lastCtx = _imp->lastUsedCPUGLContext.lock();
        if (lastCtx) {
            return lastCtx;
        }
    }


    // Context-sharing disabled as it is not needed
    OSGLContextPtr shareContext;// _imp->cpuGLShareContext.lock();
    OSGLContextPtr newContext;
    SettingsPtr settings =  appPTR->getCurrentSettings();
    GLRendererID rendererID;
    if (settings) {
        rendererID = settings->getOpenGLCPUDriver();
    }

    // For CPU Contexts, use the threads count, we are not limited by the graphic card
    const int maxContexts = appPTR->getHardwareIdealThreadCount();

    if ( (int)_imp->cpuGLContextPool.size() < maxContexts ) {
        //  Create a new one
        newContext = OSGLContext::create( FramebufferConfig(), shareContext.get(), false /*useGPU*/, -1, -1, rendererID );
        _imp->cpuGLContextPool.insert(newContext);
    } else {
        while ((int)_imp->cpuGLContextPool.size() > maxContexts) {
            _imp->cpuGLContextPool.erase(_imp->cpuGLContextPool.begin());
        }

        // Cycle through all contexts for all renders
        OSGLContextPtr lastContext = _imp->lastUsedCPUGLContext.lock();
        if (!lastContext) {
            newContext = *_imp->cpuGLContextPool.begin();
        } else {
            std::set<OSGLContextPtr>::iterator foundLast = _imp->cpuGLContextPool.find(lastContext);
            assert( foundLast != _imp->cpuGLContextPool.end() );
            if ( foundLast == _imp->cpuGLContextPool.end() ) {
                throw std::logic_error("No context to attach");
            } else {
                std::set<OSGLContextPtr>::iterator next = foundLast;
                ++next;
                if ( next == _imp->cpuGLContextPool.end() ) {
                    next = _imp->cpuGLContextPool.begin();
                }
                newContext = *next;
            }
        }
    }

    assert(newContext);

    if (settings) {
        // Initialize once static max size props
        (void)newContext->getMaxOpenGLHeight();
        (void)newContext->getMaxOpenGLWidth();
    }

    // If this is the first context, set it as the sharing context
    if (!shareContext) {
        _imp->cpuGLShareContext = newContext;
    }

    _imp->lastUsedCPUGLContext = newContext;
    
    return newContext;

#else // !HAVE_OSMESA

    Q_UNUSED(retrieveLastContext);
    
    return OSGLContextPtr();
#endif
}

void
GPUContextPool::releaseCPUGLContextFromRender(const OSGLContextPtr& context)
{
    Q_UNUSED(context);
}

NATRON_NAMESPACE_EXIT;

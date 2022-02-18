/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include "GPUContextPool.h"

#include <set>
#include <stdexcept>

#include <boost/make_shared.hpp>

#include <QMutex>
#include <QWaitCondition>

#include "Engine/AppManager.h"
#include "Engine/OSGLContext.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER

struct GPUContextPoolPrivate
{
    mutable QMutex contextPoolMutex;

    // protected by contextPoolMutex
    std::set<OSGLContextPtr> glContextPool;

#ifdef NATRON_RENDER_SHARED_CONTEXT
    OSGLContextWPtr lastUsedGLContext;
#else
    QWaitCondition glContextPoolEmpty;
    std::set<attachedGLContexts> attachedGLContexts;
#endif

    // The OpenGL context to use for sharing
    OSGLContextWPtr glShareContext;

    int currentOpenGLRendererMaxTexSize;


    GPUContextPoolPrivate()
        : contextPoolMutex()
        , glContextPool()
#ifdef NATRON_RENDER_SHARED_CONTEXT
        , lastUsedGLContext()
#else
        , glContextPoolEmpty()
        , attachedGLContexts()
#endif
        , glShareContext()
        , currentOpenGLRendererMaxTexSize(0)
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


int
GPUContextPool::getCurrentOpenGLRendererMaxTextureSize() const
{
    QMutexLocker k(&_imp->contextPoolMutex);
    return _imp->currentOpenGLRendererMaxTexSize;
}

OSGLContextPtr
GPUContextPool::attachGLContextToRender(bool checkIfGLLoaded)
{
    if (checkIfGLLoaded && (!appPTR->isOpenGLLoaded() || !appPTR->getCurrentSettings()->isOpenGLRenderingEnabled())) {
        return OSGLContextPtr();
    }
    QMutexLocker k(&_imp->contextPoolMutex);

    // Context-sharing disabled as it is not needed
    OSGLContextPtr shareContext;// _imp->glShareContext.lock();
    OSGLContextPtr newContext;
    SettingsPtr settings =  appPTR->getCurrentSettings();
    GLRendererID rendererID;
    if (settings) {
        rendererID = settings->getActiveOpenGLRendererID();
    }

    int maxContexts = settings ? std::max(settings->getMaxOpenGLContexts(), 1) : 1;

#ifndef NATRON_RENDER_SHARED_CONTEXT
    while (_imp->glContextPool.empty() && (int)_imp->attachedGLContexts.size() >= maxContexts) {
        _imp->glContextPoolEmpty.wait(k.mutex());
    }
    if ( _imp->glContextPool.empty() ) {
        assert( (int)_imp->attachedGLContexts.size() < maxContexts );
        //  Create a new one
        newContext = boost::make_shared<OSGLContext>( FramebufferConfig(), shareContext.get(), GLVersion.major, GLVersion.minor, rendererID );
    } else {
        std::set<OSGLContextPtr>::iterator it = _imp->glContextPool.begin();
        newContext = *it;
        assert(newContext);
        _imp->glContextPool.erase(it);
    }
#else

    if ( (int)_imp->glContextPool.size() < maxContexts ) {
        //  Create a new one
        newContext = boost::make_shared<OSGLContext>( FramebufferConfig(), shareContext.get(), GLVersion.major, GLVersion.minor, rendererID );
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

#endif //NATRON_RENDER_SHARED_CONTEXT
    assert(newContext);

    if (settings) {
        if (!_imp->currentOpenGLRendererMaxTexSize) {
            newContext->setContextCurrentNoRender();
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_imp->currentOpenGLRendererMaxTexSize);
        }
    }

    // If this is the first context, set it as the sharing context
    if (!shareContext) {
        _imp->glShareContext = newContext;
    }

#ifndef NATRON_RENDER_SHARED_CONTEXT
    _imp->attachedGLContexts.insert(newContext);
#else
    _imp->lastUsedGLContext = newContext;
#endif

    return newContext;
} // GPUContextPool::attachGLContextToRender

void
GPUContextPool::releaseGLContextFromRender(const OSGLContextPtr& context)
{
#ifndef NATRON_RENDER_SHARED_CONTEXT
    QMutexLocker k(&_imp->contextPoolMutex);

    // The thread has a context on its TLS so it must be found in the attached contexts set
    std::set<OSGLContextPtr>::iterator foundAttached = _imp->attachedGLContexts.find(context);

    assert( foundAttached != _imp->attachedGLContexts.end() );
    if ( foundAttached != _imp->attachedGLContexts.end() ) {
        // Re-insert back into the contextPool so it can be re-used
        _imp->glContextPool.insert(*foundAttached);
        _imp->attachedGLContexts.erase(foundAttached);

        // Wake-up one thread waiting in attachContextToThread().
        // No need to wake all threads because each thread releasing a context will wake up one thread.
        _imp->glContextPoolEmpty.wakeOne();
    }
#else
    Q_UNUSED(context);
#endif
}

NATRON_NAMESPACE_EXIT

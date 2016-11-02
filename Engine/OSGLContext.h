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

#ifndef OSGLCONTEXT_H
#define OSGLCONTEXT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif



#include <cstddef>
#include <string>
#include <vector>
#include <list>


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Engine/OSGLFramebufferConfig.h"
#include "Engine/GLShader.h"
#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER;


/**
 * @brief This class encapsulates a cross-platform OpenGL context used for offscreen rendering.
 **/
struct OSGLContextPrivate;
class OSGLContext
    : public boost::noncopyable
{
public:

    /**
     * @brief Creates a new OpenGL context for offscreen rendering. The constructor may throw an exception if the context
     * creation failed.
     * The context must be made current with makeContextCurrent before being ready to use.
     **/
    explicit OSGLContext(const FramebufferConfig& pixelFormatAttrs,
                         const OSGLContext* shareContext,
                         bool useGPUContext,
                         int major = -1,
                         int minor = -1,
                         const GLRendererID& rendererID = GLRendererID(),
                         bool coreProfile = false);

    virtual ~OSGLContext();

    /**
     * @brief This function checks that the context has at least the OpenGL version required by Natron, otherwise
     * it fails by throwing an exception with the required version.
     * Note: the context must be made current before calling this function
     **/
    static void checkOpenGLVersion(bool gpuAPI);

    /**
     * @brief Returns whether this is a GPU or CPU OpenGL context
     **/
    bool isGPUContext() const;

    /**
     * @brief Returns minimum of GL_MAX_TEXTURE_SIZE (and  OSMESA_MAX_WIDTH/OSMESA_MAX_HEIGHT on OSMesa)
     * Context needs to be bound.
     **/
    int getMaxOpenGLWidth();
    int getMaxOpenGLHeight();


    unsigned int getOrCreatePBOId();

    unsigned int getOrCreateFBOId();


    // Helper functions used by platform dependent implementations
    static bool stringInExtensionString(const char* string, const char* extensions);
    static const FramebufferConfig& chooseFBConfig(const FramebufferConfig& desired, const std::vector<FramebufferConfig>& alternatives, int count);

    GLShaderBasePtr getOrCreateCopyTexShader();

    GLShaderBasePtr getOrCreateFillShader();

    GLShaderBasePtr getOrCreateMaskMixShader(bool maskEnabled, bool maskInvert);

    GLShaderBasePtr getOrCreateCopyUnprocessedChannelsShader(bool doR,
                                                             bool doG,
                                                             bool doB,
                                                             bool doA);

    /**
     * @brief Same as setContextCurrent() except that it should be used to bind the context to perform NON-RENDER operations!
     **/
    void setContextCurrentNoRender(int width = 0, int height = 0, int rowWidth = 0, void* buffer = 0);
    void unsetCurrentContextNoRender(bool useGPU);

    static void unsetCurrentContextNoRenderInternal(bool useGPU, const OSGLContext* context);

    /**
     * @brief Returns all renderers capable of rendering OpenGL
     **/
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

private:


    /*  @brief Makes the context current for the calling
     *  thread. A context can only be made current on
     *  a single thread at a time and each thread can have only a single current
     *  context at a time.
     *
     *  @thread_safety This function may be called from any thread.
     */
    void setContextCurrent_GPU(const AbortableRenderInfoPtr& render
#ifdef DEBUG
                           , double frameTime
#endif
                           );


    void setContextCurrent_CPU(const AbortableRenderInfoPtr& render
#ifdef DEBUG
                               , double frameTime
#endif
                               , int width
                               , int height
                               , int rowWidth
                               , void* buffer);

    void setContextCurrentInternal(const AbortableRenderInfoPtr& render
#ifdef DEBUG
                                   , double frameTime
#endif
                                   , int width
                                   , int height
                                   , int rowWidth
                                   , void* buffer);

    /**
     * @brief Releases the OpenGL context from this thread.
     * @param unlockContext If true, the context will be made available for other renders as well
     **/
    void unsetCurrentContext(const AbortableRenderInfoPtr& abortInfo);


    friend class OSGLContextAttacher;
    boost::scoped_ptr<OSGLContextPrivate> _imp;
};


/**
 * @brief RAII style class to safely call setContextCurrent() and unsetCurrentContext()
 **/
class OSGLContextAttacher
{
    OSGLContextPtr _c;
    AbortableRenderInfoWPtr _a;
#ifdef DEBUG
    double _frameTime;
#endif
    bool _attached;
    int _width;
    int _height;
    int _rowWidth;
    void* _buffer;

public:

    /**
     * @brief Locks the given context to this render. The context MUST be a GPU OpenGL context.
     **/
    OSGLContextAttacher(const OSGLContextPtr& c,
                        const AbortableRenderInfoPtr& render
#ifdef DEBUG
                        ,
                        double frameTime
#endif
                        )
    : _c(c)
    , _a(render)
#ifdef DEBUG
    , _frameTime(frameTime)
#endif
    , _attached(false)
    , _width(0)
    , _height(0)
    , _rowWidth(0)
    , _buffer(0)
    {
        assert(c);
    }

    /**
     * @brief Locks the given context to this render. The context MUST be a CPU OpenGL context.
     **/
    OSGLContextAttacher(const OSGLContextPtr& c,
                        const AbortableRenderInfoPtr& render
#ifdef DEBUG
                        ,
                        double frameTime
#endif
                        , int width, int height, int rowWidth, void* buffer
    )
    : _c(c)
    , _a(render)
#ifdef DEBUG
    , _frameTime(frameTime)
#endif
    , _attached(false)
    , _width(width)
    , _height(height)
    , _rowWidth(rowWidth)
    , _buffer(buffer)
    {
        assert(c);
    }

    OSGLContextPtr getContext() const
    {
        return _c;
    }

    void attach()
    {
        if (!_attached) {
            if (!_c->isGPUContext()) {
                _c->setContextCurrent_CPU(_a.lock()
#ifdef DEBUG
                                          , _frameTime
#endif
                                          , _width
                                          , _height
                                          , _rowWidth
                                          , _buffer);
            } else {
                _c->setContextCurrent_GPU(_a.lock()
#ifdef DEBUG
                                          , _frameTime
#endif
                                          );
            }
            _attached = true;
        }
    }

    void dettach()
    {

        if (_attached) {
            _c->unsetCurrentContext( _a.lock() );
            _attached = false;
        }
    }


    ~OSGLContextAttacher()
    {
        dettach();
    }
};

NATRON_NAMESPACE_EXIT;

#endif // OSGLCONTEXT_H

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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
#include <boost/enable_shared_from_this.hpp>
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
    , public boost::enable_shared_from_this<OSGLContext>
{
private:

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

public:

    static OSGLContextPtr create(const FramebufferConfig& pixelFormatAttrs,
                          const OSGLContext* shareContext,
                          bool useGPUContext,
                          int major = -1,
                          int minor = -1,
                          const GLRendererID& rendererID = GLRendererID(),
                          bool coreProfile = false)
    {
        OSGLContextPtr ret(new OSGLContext(pixelFormatAttrs, shareContext, useGPUContext, major, minor, rendererID, coreProfile));
        return ret;
    }


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



    static void unsetCurrentContextNoRenderInternal(bool useGPU, const OSGLContext* context);

    /**
     * @brief Returns all renderers capable of rendering OpenGL
     **/
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

    QThread* getCurrentThread() const;

private:

    void setContextCurrentInternal(int width, int height, int rowWidth, void* buffer);

    /**
     * @brief Releases the OpenGL context from this thread.
     **/
    void unsetCurrentContext();


    friend class OSGLContextAttacher;
    boost::scoped_ptr<OSGLContextPrivate> _imp;
};


/**
 * @brief RAII style class to safely ensure a context is current to the calling thread.
 * This can be created recursively on the same thread, however 2 threads cannot concurrently own the context.
 **/
class OSGLContextAttacher : public boost::enable_shared_from_this<OSGLContextAttacher>
{
    OSGLContextPtr _c;

    bool _attached;
    int _width;
    int _height;
    int _rowWidth;
    void* _buffer;

    /**
     * @brief Locks the given context to this thread.
     **/
    OSGLContextAttacher(const OSGLContextPtr& c);

    /**
     * @brief Locks the given context to this thread. The context MUST be a CPU OpenGL context.
     **/
    OSGLContextAttacher(const OSGLContextPtr& c, int width, int height, int rowWidth, void* buffer);


public:

    /**
     * @brief Create an attacher object. If an attacher object already exists on this thread local storage it will
     * be retrieved instead of creating a new one. 
     * When leaving the ctor, the context is not necessarily attached, to ensure it is correctly attached, call attach().
     **/
    static OSGLContextAttacherPtr create(const OSGLContextPtr& c);

    /**
     * @brief Same as create(c) but also provides the default framebuffer for OSMesa contexts.
     **/
    static OSGLContextAttacherPtr create(const OSGLContextPtr& c, int width, int height, int rowWidth, void* buffer);

    /**
     * @brief Ensures the dettach() function is called if isAttached() returns true
     **/
    ~OSGLContextAttacher();

    OSGLContextPtr getContext() const;

    /**
     * @brief Attaches the context to the current thread. Does nothing if already attached.
     **/
    void attach();

    /**
     * @brief Returns true if the context is attached
     **/
    bool isAttached() const;

    /**
     * @brief Dettaches the context from the current thread. Does nothing if already dettached.
     * This is called from the dtor of this object and does not need to be called explicitly.
     **/
    void dettach();

};

/**
 * @brief RAII style class to help switching temporarily context on a thread.
 * The ctor will save the current context while the dtor will dettach whatever context
 * was made current in-between and make the original context prior to the ctor current again.
 **/
class OSGLContextSaver
{
    OSGLContextAttacherPtr savedContext;

public:

    OSGLContextSaver();

    ~OSGLContextSaver();
};

NATRON_NAMESPACE_EXIT;

#endif // OSGLCONTEXT_H

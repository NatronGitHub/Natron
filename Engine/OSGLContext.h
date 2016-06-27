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




#include <cstddef>
#include <string>
#include <vector>
#include <list>


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Engine/OSGLFramebufferConfig.h"
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
                         int major = GLVersion.major,
                         int minor = GLVersion.minor,
                         const GLRendererID& rendererID = GLRendererID(),
                         bool coreProfile = false);

    virtual ~OSGLContext();

    /**
     * @brief This function checks that the context has at least the OpenGL version required by Natron, otherwise
     * it fails by throwing an exception with the required version.
     * Note: the context must be made current before calling this function
     **/
    static void checkOpenGLVersion();


    GLuint getPBOId() const;

    GLuint getFBOId() const;

    // Helper functions used by platform dependent implementations
    static bool stringInExtensionString(const char* string, const char* extensions);
    static const FramebufferConfig& chooseFBConfig(const FramebufferConfig& desired, const std::vector<FramebufferConfig>& alternatives, int count);


    /**
     * @brief Returns one of the built-in shaders, used in the Image class.
     * Note: this context must be made current before calling this function
     **/
    boost::shared_ptr<GLShader> getOrCreateFillShader();
    boost::shared_ptr<GLShader> getOrCreateMaskMixShader(bool maskEnabled);
    boost::shared_ptr<GLShader> getOrCreateCopyUnprocessedChannelsShader(bool doR, bool doG, bool doB, bool doA);

    /**
     * @brief Same as setContextCurrent() except that it should be used to bind the context to perform NON-RENDER operations!
     **/
    void setContextCurrentNoRender();
    static void unsetCurrentContextNoRender();

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
    void setContextCurrent(const AbortableRenderInfoPtr& render
#ifdef DEBUG
                           , double frameTime
#endif
                           );

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
public:

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
    {

    }

    void attach()
    {
        if (!_attached) {
            _c->setContextCurrent(_a.lock()
#ifdef DEBUG
                                 , _frameTime
#endif
                                 );
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

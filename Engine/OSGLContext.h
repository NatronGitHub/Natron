/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include <boost/weak_ptr.hpp>
#endif

#include <cstddef>
#include <string>
#include <vector>
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Engine/OSGLFramebufferConfig.h"
#include "Engine/GLShader.h"
#include "Engine/RectI.h"
#include "Global/GLIncludes.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


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

protected:

    /**
     * @brief Convenience ctor for derived classes
     **/
    OSGLContext(bool useGPUContext);

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



    static void unsetCurrentContextNoRenderInternal(bool useGPU, OSGLContext* context);

    /**
     * @brief Returns all renderers capable of rendering OpenGL
     **/
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

    QThread* getCurrentThread() const;

    /**
     * @brief Helper function to setup the OpenGL viewport when doing processing on a texture
     **/
    template <typename GL>
    static void setupGLViewport(const RectI& bounds, const RectI& roi)
    {
        GL::Viewport( roi.x1 - bounds.x1, roi.y1 - bounds.y1, roi.width(), roi.height() );
        glCheckError(GL);
        GL::MatrixMode(GL_PROJECTION);
        GL::LoadIdentity();
        GL::Ortho( roi.x1, roi.x2,
                  roi.y1, roi.y2,
                  -10.0 * (roi.y2 - roi.y1), 10.0 * (roi.y2 - roi.y1) );
        glCheckError(GL);
        GL::MatrixMode(GL_MODELVIEW);
        GL::LoadIdentity();
    } // setupGLViewport

    /**
     * @brief Helper function to apply texture mapping assuming both in and out textures are bounded to the context.
     **/
    template <typename GL>
    static void applyTextureMapping(const RectI& srcBounds, const RectI& dstBounds, const RectI& roi)
    {
        setupGLViewport<GL>(dstBounds, roi);

        // Compute the texture coordinates to match the srcRoi
        Point srcTexCoords[4], vertexCoords[4];
        vertexCoords[0].x = roi.x1;
        vertexCoords[0].y = roi.y1;
        srcTexCoords[0].x = (roi.x1 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[0].y = (roi.y1 - srcBounds.y1) / (double)srcBounds.height();

        vertexCoords[1].x = roi.x2;
        vertexCoords[1].y = roi.y1;
        srcTexCoords[1].x = (roi.x2 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[1].y = (roi.y1 - srcBounds.y1) / (double)srcBounds.height();

        vertexCoords[2].x = roi.x2;
        vertexCoords[2].y = roi.y2;
        srcTexCoords[2].x = (roi.x2 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[2].y = (roi.y2 - srcBounds.y1) / (double)srcBounds.height();

        vertexCoords[3].x = roi.x1;
        vertexCoords[3].y = roi.y2;
        srcTexCoords[3].x = (roi.x1 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[3].y = (roi.y2 - srcBounds.y1) / (double)srcBounds.height();

        GL::Begin(GL_POLYGON);
        for (int i = 0; i < 4; ++i) {
            GL::TexCoord2d(srcTexCoords[i].x, srcTexCoords[i].y);
            GL::Vertex2d(vertexCoords[i].x, vertexCoords[i].y);
        }
        GL::End();
        glCheckError(GL);
    } // applyTextureMapping

private:

    void setContextCurrentInternal(int width, int height, int rowWidth, void* buffer);

protected:

    virtual void makeGPUContextCurrent();

    virtual void unsetGPUContext();

private:


    /**
     * @brief Releases the OpenGL context from this thread.
     **/
    void unsetCurrentContext();

    bool hasCreatedContext() const;


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

    int _attached;
    int _width;
    int _height;
    int _rowWidth;
    void* _buffer;
    bool _dettachOnDtor;

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
     * @brief Set whether to call dettach in dtor. By default its true
     **/
    void setDettachOnDtor(bool enabled);

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

NATRON_NAMESPACE_EXIT

#endif // OSGLCONTEXT_H

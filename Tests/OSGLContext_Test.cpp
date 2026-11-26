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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#include <gtest/gtest.h>

#include "Global/GLIncludes.h"
#include "Engine/OSGLContext.h"
#include "Engine/GPUContextPool.h"
#include "Engine/Settings.h"

#include "TestUtils.h"

NATRON_NAMESPACE_USING

#if defined(__NATRON_WIN32__) && !defined(__NATRON_MINGW__)
namespace {

// Define nullptr_t operator since MSVC dosn't appear to have one by default.
std::ostream& operator<<(std::ostream& os, const std::nullptr_t p) {
    return os << "<nullptr>";
}

}  // namespace
#endif

TEST(OSGLContext, Basic)
{
    if (!appPTR->isOpenGLLoaded()) {
        NatronTest::skip("OpenGL loading failed");
        return;
    }

    SettingsPtr settings =  appPTR->getCurrentSettings();
    ASSERT_NE(settings.get(), nullptr);
    GLRendererID rendererID = settings->getActiveOpenGLRendererID();

    // Verify that we start without a context being set.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());

    auto newContext = std::make_shared<OSGLContext>( FramebufferConfig(), nullptr, GLVersion.major, GLVersion.minor, rendererID);

    // Verify that creating a context does not set the current context.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());

    newContext->setContextCurrentNoRender();

    // Verify that the current context was actually set.
    EXPECT_TRUE(OSGLContext::threadHasACurrentContext());

    OSGLContext::unsetCurrentContextNoRender();

    // Verify that it is no longer set.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());
}

TEST(GPUContextPool, Basic) {
    if (!appPTR->isOpenGLLoaded()) {
        NatronTest::skip("OpenGL loading failed");
        return;
    }

    // Verify that we start without a context being set.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());

    GPUContextPool pool;

    // Verify that creating the pool does not set the current context.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());
    OSGLContextPtr context = pool.attachGLContextToRender(/* checkIfGLLoaded */ false);

    // Verify that the attach returns a valid pointer to a context, but
    // this context has not been made current on this thread yet.
    EXPECT_NE(context.get(), nullptr);
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());

    context->setContextCurrentNoRender();

    // Verify that the current context is now set.
    EXPECT_TRUE(OSGLContext::threadHasACurrentContext());

    OSGLContext::unsetCurrentContextNoRender();

    // Verify that the current context is no longer set.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());

    pool.releaseGLContextFromRender(context);

    // Verify that returing the context does not make it current.
    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());

    context.reset();

    EXPECT_FALSE(OSGLContext::threadHasACurrentContext());
}

// OSGLContext::getGPUInfos() creates a context per renderer and makes it current: the context of the caller (e.g. a
// QOpenGLWidget, whose initializeGL() calls AppManager::initializeOpenGLFunctionsOnce(), which updates the About
// window) must be current again when it returns (#1502).
TEST(OSGLContext, GetGPUInfosRestoresCurrentContext)
{
    // Without a renderer that getGPUInfos() accepts, it makes no context current and the test would check nothing:
    // e.g. on the Linux CI (Xvfb, Mesa llvmpipe), OSGLContext_x11::getGPUInfos() skips the renderers that are not
    // accelerated before creating a context.
    if ( !appPTR->isOpenGLLoaded() || appPTR->getOpenGLRenderers().empty() ) {
        NatronTest::skip("no OpenGL renderer that getGPUInfos() uses");

        return;
    }

    GPUContextPool pool;
    OSGLContextPtr context = pool.attachGLContextToRender(/* checkIfGLLoaded */ false);
    ASSERT_NE(context.get(), nullptr);
    context->setContextCurrentNoRender();
    // no context left current for the next tests, also when an assertion fails
    struct ContextReleaser
    {
        GPUContextPool& pool;
        OSGLContextPtr context;

        ~ContextReleaser()
        {
            OSGLContext::unsetCurrentContextNoRender();
            pool.releaseGLContextFromRender(context);
        }
    } releaser{pool, context};
    ASSERT_TRUE(OSGLContext::threadHasACurrentContext());
    // a texture of this context, unknown to the (unshared) contexts that getGPUInfos() creates
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindTexture(GL_TEXTURE_2D, 0);
    ASSERT_EQ(glIsTexture(texture), GL_TRUE);

    std::list<OpenGLRendererInfo> renderers;
    OSGLContext::getGPUInfos(renderers);

    ASSERT_TRUE(OSGLContext::threadHasACurrentContext());
    EXPECT_EQ(glIsTexture(texture), GL_TRUE) << "another context is current after getGPUInfos()";
    glDeleteTextures(1, &texture);
}

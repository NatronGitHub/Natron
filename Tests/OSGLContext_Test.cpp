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

#include <gtest/gtest.h>

#include "Engine/OSGLContext.h"
#include "Engine/GPUContextPool.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_USING

TEST(OSGLContext, Basic)
{
    if (!appPTR->isOpenGLLoaded()) {
        // TODO: Convert to GTEST_SKIP() when gtest updated.
        std::cerr << "Skipping test because OpenGL loading failed." << std::endl;
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
        /// TODO: Convert to GTEST_SKIP() when gtest updated.
        std::cerr << "Skipping test because OpenGL loading failed." << std::endl;
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

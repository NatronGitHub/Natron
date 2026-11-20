/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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


// A plug-in that supports OpenGL render but sets kOfxImageEffectPropCPURenderSupported (OFX 1.5.1) to
// "false" can only render with OpenGL: Natron must treat it like an OpenGL render support of "needed".

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <ofxGPURender.h>
#include <ofxhPropertySuite.h>

#include "Engine/OfxHost.h"

NATRON_NAMESPACE_USING

namespace {
// The property, as HostSupport declares it on every image effect descriptor (and so on every instance).
const OFX::Host::Property::PropSpec effectProps[] = {
    { kOfxImageEffectPropCPURenderSupported, OFX::Host::Property::eString, 1, false, "true" },
    OFX::Host::Property::propSpecEnd
};

PluginOpenGLRenderSupport
apply(PluginOpenGLRenderSupport openGLSupport,
      const char* cpuRenderSupported)
{
    OFX::Host::Property::Set props(effectProps);

    if (cpuRenderSupported) {
        props.setStringProperty(kOfxImageEffectPropCPURenderSupported, cpuRenderSupported);
    }

    return NATRON_NAMESPACE::OfxHost::applyCPURenderSupport(openGLSupport, props);
}
}

// The default ("true", what every plug-in written before OFX 1.5.1 gets) changes nothing.
TEST(OfxCPURenderSupport, Default)
{
    EXPECT_EQ( ePluginOpenGLRenderSupportNone, apply(ePluginOpenGLRenderSupportNone, NULL) );
    EXPECT_EQ( ePluginOpenGLRenderSupportYes, apply(ePluginOpenGLRenderSupportYes, NULL) );
    EXPECT_EQ( ePluginOpenGLRenderSupportNeeded, apply(ePluginOpenGLRenderSupportNeeded, NULL) );
    EXPECT_EQ( ePluginOpenGLRenderSupportYes, apply(ePluginOpenGLRenderSupportYes, "true") );
}

TEST(OfxCPURenderSupport, NoCPURender)
{
    // OpenGL is the only way left
    EXPECT_EQ( ePluginOpenGLRenderSupportNeeded, apply(ePluginOpenGLRenderSupportYes, "false") );
    EXPECT_EQ( ePluginOpenGLRenderSupportNeeded, apply(ePluginOpenGLRenderSupportNeeded, "false") );
    // no OpenGL either (e.g. a CUDA-, Metal- or OpenCL-only plug-in): Natron has no GPU API to offer
    EXPECT_EQ( ePluginOpenGLRenderSupportNone, apply(ePluginOpenGLRenderSupportNone, "false") );
}

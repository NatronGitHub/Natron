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

// A plug-in that returns kOfxStatGPURenderFailed (formerly kOfxStatGLRenderFailed) from an OpenGL render
// is rendered again on the CPU: "If a plug-in returns kOfxStatGLRenderFailed, the host should retry the
// render with OpenGL rendering disabled" (ofxGPURender.h, #1359). A plug-in that can't render on the
// CPU (kOfxImageEffectPropCPURenderSupported "false") gets an error instead.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/KnobTypes.h"
#include "Engine/OSGLContext.h"
#include "Engine/Settings.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// Enable OpenGL rendering (Preferences/GPU Rendering) for the lifetime of the object.
class OpenGLRenderingEnabler
{
    KnobChoice* _knob;
    int _savedValue;

public:
    OpenGLRenderingEnabler()
        : _knob( dynamic_cast<KnobChoice*>( appPTR->getCurrentSettings()->getKnobByName("enableOpenGLRendering").get() ) )
        , _savedValue(0)
    {
        if (_knob) {
            _savedValue = _knob->getValue();
            _knob->setValue( (int)Settings::eEnableOpenGLEnabled );
        }
    }

    ~OpenGLRenderingEnabler()
    {
        if (_knob) {
            _knob->setValue(_savedValue);
        }
    }

    bool isEnabled() const
    {
        return appPTR->getCurrentSettings()->isOpenGLRenderingEnabled();
    }
};

bool
canRunTest(const char* pluginID)
{
    if ( !NatronTestPlugin::isAvailable(pluginID) || !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(pluginID);

        return false;
    }
    if ( !appPTR->isOpenGLLoaded() ) {
        // TODO: Convert to GTEST_SKIP() when gtest updated.
        std::cerr << "[  SKIPPED ] " << pluginID << ": OpenGL could not be loaded" << std::endl;

        return false;
    }
    if ( !appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering) || appPTR->getOpenGLRenderers().empty() ) {
        // OpenGL rendering can't be enabled in the preferences (the setting is hidden and
        // Settings::isOpenGLRenderingEnabled() is false) when no renderer qualifies. On Linux, the
        // renderers that GLX_MESA_query_renderer reports as not accelerated are skipped, e.g. the
        // software Mesa llvmpipe of the Xvfb CI (OSGLContext_x11::getGPUInfos).
        // TODO: Convert to GTEST_SKIP() when gtest updated.
        std::cerr << "[  SKIPPED ] " << pluginID << ": no OpenGL renderer can be used for OpenGL rendering" << std::endl;

        return false;
    }

    return true;
}
}

TEST(OpenGLRenderFailed, RetriedOnTheCPU)
{
    if ( !canRunTest(kNatronTestPluginGPURenderFailed) ) {
        return;
    }
    OpenGLRenderingEnabler enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    NodePtr source = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginGPURenderFailed);
    ASSERT_TRUE( source && node );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, node->getCurrentOpenGLRenderSupport() );
    ASSERT_TRUE( node->getApp()->getProject()->connectNodes(0, source, node) );

    const int glRenders = NatronTestPlugin::counter("GPURenderFailed.glRenders");
    const int cpuRenders = NatronTestPlugin::counter("GPURenderFailed.cpuRenders");
    ImagePtr image = NatronTestPlugin::renderFrame(node, 1.);
    // the OpenGL render was tried, and failed
    EXPECT_EQ( glRenders + 1, NatronTestPlugin::counter("GPURenderFailed.glRenders") );
    // then the frame was rendered on the CPU
    EXPECT_TRUE( bool(image) ) << "the render failed instead of being retried on the CPU";
    EXPECT_EQ( cpuRenders + 1, NatronTestPlugin::counter("GPURenderFailed.cpuRenders") );
    if (image) {
        EXPECT_EQ( eStorageModeRAM, image->getStorageMode() );
    }

    node->destroyNode(true, false);
    source->destroyNode(true, false);
}

TEST(OpenGLRenderFailed, NoCPURenderIsAnError)
{
    if ( !canRunTest(kNatronTestPluginGPUOnlyRenderFailed) ) {
        return;
    }
    OpenGLRenderingEnabler enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    NodePtr source = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginGPUOnlyRenderFailed);
    ASSERT_TRUE( source && node );
    ASSERT_EQ( ePluginOpenGLRenderSupportNeeded, node->getCurrentOpenGLRenderSupport() );
    ASSERT_TRUE( node->getApp()->getProject()->connectNodes(0, source, node) );

    const int glRenders = NatronTestPlugin::counter("GPUOnlyRenderFailed.glRenders");
    const int cpuRenders = NatronTestPlugin::counter("GPUOnlyRenderFailed.cpuRenders");
    EXPECT_FALSE( bool( NatronTestPlugin::renderFrame(node, 1.) ) );
    EXPECT_EQ( glRenders + 1, NatronTestPlugin::counter("GPUOnlyRenderFailed.glRenders") );
    // never on the CPU (the user is told why in a persistent message, which is only printed in background
    // mode)
    EXPECT_EQ( cpuRenders, NatronTestPlugin::counter("GPUOnlyRenderFailed.cpuRenders") );

    node->destroyNode(true, false);
    source->destroyNode(true, false);
}

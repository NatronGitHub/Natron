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
// The same for kOfxStatErrMemory, which a plug-in returns when it can't load an input texture ("The plug-in
// should abort the GL render and return kOfxStatErrMemory, after which the host can decide to retry the
// operation with CPU based processing", clipLoadTexture), and for an input that can't be loaded into a
// texture, whatever the plug-in returns (#1945).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/OSGLContext.h"
#include "Engine/Settings.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
bool
canRunTest(const char* pluginID)
{
    for (const char* id : { pluginID, kNatronTestPluginGenerator }) {
        if ( !NatronTestPlugin::isAvailable(id) ) {
            NatronTestPlugin::reportMissing(id);

            return false;
        }
    }

    return NatronTest::requireOpenGLRendering();
}

// The counters of an OpenGL test plug-in, read before and after a render.
struct GLCounters
{
    explicit GLCounters(const std::string& id)
        : id(id)
        , glRenders( NatronTestPlugin::counter(id + ".glRenders") )
        , cpuRenders( NatronTestPlugin::counter(id + ".cpuRenders") )
        , textureOK( NatronTestPlugin::counter(id + ".textureOK") )
        , textureErrMemory( NatronTestPlugin::counter(id + ".textureErrMemory") )
    {
    }

    // the increments since construction
    int glRendersSince() const { return NatronTestPlugin::counter(id + ".glRenders") - glRenders; }
    int cpuRendersSince() const { return NatronTestPlugin::counter(id + ".cpuRenders") - cpuRenders; }
    int textureOKSince() const { return NatronTestPlugin::counter(id + ".textureOK") - textureOK; }
    int textureErrMemorySince() const { return NatronTestPlugin::counter(id + ".textureErrMemory") - textureErrMemory; }

    const std::string id;
    const int glRenders, cpuRenders, textureOK, textureErrMemory;
};

// A net.natron.test.Generator connected to the input of a new node of pluginID; wide sets the generator's
// region of definition to (0, 0, 2^17, 1), wider than any GL_MAX_TEXTURE_SIZE.
std::vector<NodePtr>
createGraph(const char* pluginID,
            bool wide)
{
    NodePtr source = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr node = NatronTestPlugin::createNode(pluginID);

    if ( !source || !node || !node->getApp()->getProject()->connectNodes(0, source, node) ) {
        return std::vector<NodePtr>();
    }
    if (wide) {
        KnobBoolPtr knob = std::dynamic_pointer_cast<KnobBool>( source->getKnobByName("wide") );
        if (!knob) {
            return std::vector<NodePtr>();
        }
        knob->setValue(true);
    }

    return { source, node };
}

void
destroyGraph(const std::vector<NodePtr>& graph)
{
    for (std::vector<NodePtr>::const_reverse_iterator it = graph.rbegin(); it != graph.rend(); ++it) {
        (*it)->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    }
}

// true if image is a RAM image whose pixel (0, 0) is 1 in all components: the CPU render of the test
// plug-ins, not the black frame of their OpenGL render
bool
isCPURender(const ImagePtr& image)
{
    if ( !image || (image->getStorageMode() != eStorageModeRAM) || !image->getBounds().contains(0, 0) ) {
        return false;
    }
    Image::ReadAccess acc( image.get() );
    const float* p = (const float*)acc.pixelAt(0, 0);

    return p && (p[0] == 1.f) && (p[1] == 1.f) && (p[2] == 1.f) && (p[3] == 1.f);
}
}

TEST(OpenGLRenderFailed, RetriedOnTheCPU)
{
    if ( !canRunTest(kNatronTestPluginGPURenderFailed) ) {
        return;
    }
    NatronTest::ScopedOpenGLRendering enabler;
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
    NatronTest::ScopedOpenGLRendering enabler;
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

// kOfxStatErrMemory from the OpenGL render: the frame is rendered again on the CPU.
TEST(OpenGLRenderFailed, ErrMemoryRetriedOnTheCPU)
{
    if ( !canRunTest(kNatronTestPluginGLErrMemory) ) {
        return;
    }
    NatronTest::ScopedOpenGLRendering enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    const std::vector<NodePtr> graph = createGraph(kNatronTestPluginGLErrMemory, false);
    ASSERT_EQ( 2u, graph.size() );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, graph[1]->getCurrentOpenGLRenderSupport() );

    const GLCounters counters("GLErrMemory");
    ImagePtr image = NatronTestPlugin::renderFrame(graph[1], 1.);
    EXPECT_EQ( 1, counters.glRendersSince() );
    EXPECT_EQ( 1, counters.cpuRendersSince() ) << "the render was not retried on the CPU";
    EXPECT_TRUE( isCPURender(image) );

    destroyGraph(graph);
}

// kOfxStatErrMemory from the OpenGL render of a plug-in that can't render on the CPU: the render fails.
TEST(OpenGLRenderFailed, ErrMemoryNoCPURenderIsAnError)
{
    if ( !canRunTest(kNatronTestPluginGLOnlyErrMemory) ) {
        return;
    }
    NatronTest::ScopedOpenGLRendering enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    const std::vector<NodePtr> graph = createGraph(kNatronTestPluginGLOnlyErrMemory, false);
    ASSERT_EQ( 2u, graph.size() );
    ASSERT_EQ( ePluginOpenGLRenderSupportNeeded, graph[1]->getCurrentOpenGLRenderSupport() );

    const GLCounters counters("GLOnlyErrMemory");
    EXPECT_FALSE( bool( NatronTestPlugin::renderFrame(graph[1], 1.) ) );
    EXPECT_EQ( 1, counters.glRendersSince() );
    // (the user is told why in a persistent message, which is only printed in background mode)
    EXPECT_EQ( 0, counters.cpuRendersSince() );

    destroyGraph(graph);
}

// An input too large for a texture, asked for in getRegionsOfInterest: Natron can't load it before the OpenGL
// render, and renders the frame on the CPU, instead of giving the plug-in no texture or a broken one (which made
// it render a black frame).
TEST(OpenGLRenderFailed, InputTextureFailedRetriedOnTheCPU)
{
    if ( !canRunTest(kNatronTestPluginGLTextureFailed) ) {
        return;
    }
    NatronTest::ScopedOpenGLRendering enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    const std::vector<NodePtr> graph = createGraph(kNatronTestPluginGLTextureFailed, true);
    ASSERT_EQ( 2u, graph.size() );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, graph[1]->getCurrentOpenGLRenderSupport() );

    const GLCounters counters("GLTextureFailed");
    ImagePtr image = NatronTestPlugin::renderFrame(graph[1], 1.);
    EXPECT_EQ( 0, counters.textureOKSince() ) << "the plug-in got a texture of an input larger than GL_MAX_TEXTURE_SIZE";
    EXPECT_EQ( 1, counters.cpuRendersSince() ) << "the render was not retried on the CPU";
    EXPECT_TRUE( isCPURender(image) ) << "black frame";

    destroyGraph(graph);
}

// The same, with the input region asked for in the OpenGL render only (clipLoadTexture's bounds), so that Natron
// renders the input during the action: clipLoadTexture
// returns kOfxStatErrMemory, and the frame is rendered again on the CPU, although the plug-in ignores the
// failure and returns kOfxStatOK.
TEST(OpenGLRenderFailed, InputTextureFailedInRenderRetriedOnTheCPU)
{
    if ( !canRunTest(kNatronTestPluginGLTextureFailedInRender) ) {
        return;
    }
    NatronTest::ScopedOpenGLRendering enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    const std::vector<NodePtr> graph = createGraph(kNatronTestPluginGLTextureFailedInRender, true);
    ASSERT_EQ( 2u, graph.size() );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, graph[1]->getCurrentOpenGLRenderSupport() );

    const GLCounters counters("GLTextureFailedInRender");
    ImagePtr image = NatronTestPlugin::renderFrame(graph[1], 1.);
    EXPECT_EQ( 1, counters.glRendersSince() );
    EXPECT_EQ( 0, counters.textureOKSince() ) << "the plug-in got a texture of an input larger than GL_MAX_TEXTURE_SIZE";
    EXPECT_EQ( 1, counters.textureErrMemorySince() );
    EXPECT_EQ( 1, counters.cpuRendersSince() ) << "the render was not retried on the CPU";
    EXPECT_TRUE( isCPURender(image) ) << "black frame";

    destroyGraph(graph);
}

// An input too large for a texture, for a plug-in that can't render on the CPU: the render fails.
TEST(OpenGLRenderFailed, InputTextureFailedNoCPURenderIsAnError)
{
    if ( !canRunTest(kNatronTestPluginGLOnlyTextureFailed) ) {
        return;
    }
    NatronTest::ScopedOpenGLRendering enabler;
    ASSERT_TRUE( enabler.isEnabled() );
    const std::vector<NodePtr> graph = createGraph(kNatronTestPluginGLOnlyTextureFailed, true);
    ASSERT_EQ( 2u, graph.size() );
    ASSERT_EQ( ePluginOpenGLRenderSupportNeeded, graph[1]->getCurrentOpenGLRenderSupport() );

    const GLCounters counters("GLOnlyTextureFailed");
    EXPECT_FALSE( bool( NatronTestPlugin::renderFrame(graph[1], 1.) ) );
    EXPECT_EQ( 0, counters.textureOKSince() );
    // (the user is told why in a persistent message, which is only printed in background mode)
    EXPECT_EQ( 0, counters.cpuRendersSince() );

    destroyGraph(graph);
}

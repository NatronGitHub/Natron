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

// OpenGL render support (#1384):
// - when kOfxActionOpenGLContextAttached returns kOfxStatFailed, "the host should not attempt to run the
//   plug-in in OpenGL render mode" (ofxGPURender.h): the frame is rendered on the CPU, and OpenGL is not
//   tried again; a plug-in that needs OpenGL fails, without attaching again;
// - a plug-in that needs OpenGL, rendered where OpenGL rendering is disabled or unavailable, fails with a
//   message saying why, without being called.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/ParallelRenderArgs.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginOpenGL.cpp
#define kNatronTestPluginGLAttachFailed "net.natron.test.GLAttachFailed"
#define kNatronTestPluginGLAttachFailedNeeded "net.natron.test.GLAttachFailedNeeded"

NATRON_NAMESPACE_USING

namespace {
// Set OpenGL rendering (Preferences/GPU Rendering) for the lifetime of the object.
class OpenGLRenderingSetter
{
    KnobChoice* _knob;
    int _savedValue;

public:
    explicit OpenGLRenderingSetter(Settings::EnableOpenGLEnum value)
        : _knob( dynamic_cast<KnobChoice*>( appPTR->getCurrentSettings()->getKnobByName("enableOpenGLRendering").get() ) )
        , _savedValue(0)
    {
        if (_knob) {
            _savedValue = _knob->getValue();
            _knob->setValue( (int)value );
        }
    }

    ~OpenGLRenderingSetter()
    {
        if (_knob) {
            _knob->setValue(_savedValue);
        }
    }
};

bool
canRunTest(const char* pluginID,
           bool needsOpenGL)
{
    if ( !NatronTestPlugin::isAvailable(pluginID) ) {
        NatronTestPlugin::reportSkipped(pluginID);

        return false;
    }
    if ( needsOpenGL && !appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering) ) {
        const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
        std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": no OpenGL rendering" << std::endl;

        return false;
    }

    return true;
}

NodePtr
createNode(const char* pluginID)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// Render the output of node at time, view 0 and scale 1, as an interactive render. Returns the rendered
// image, or a null image if the render failed.
ImagePtr
renderFrame(const NodePtr& node,
            double time)
{
    EffectInstancePtr effect = node->getEffectInstance();
    const ViewIdx view(0);
    const unsigned int mipmapLevel = 0;
    const RenderScale scale = RenderScale::fromMipmapLevel(mipmapLevel);
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = effect->getRegionOfDefinition_public(node->getHashValue(), time, scale, view, &rod, &isProjectFormat);

    if ( (stat == eStatusFailed) || rod.isNull() ) {
        return ImagePtr();
    }
    const RectI renderWindow = rod.toPixelEnclosing( mipmapLevel, effect->getAspectRatio(-1) );
    RenderingFlagSetter flagIsRendering(node);
    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    ParallelRenderArgsSetter frameRenderArgs( time, view,
                                              true, // isRenderUserInteraction
                                              false, // isSequential
                                              abortInfo,
                                              node, // tree root
                                              0, // texture index
                                              node->getApp()->getTimeLine().get(),
                                              NodePtr(), // rotoPaint node
                                              false, // isAnalysis
                                              false, // isDraft
                                              RenderStatsPtr() );
    FrameRequestMap request;
    stat = EffectInstance::computeRequestPass(time, view, mipmapLevel, rod, node, request);
    if (stat == eStatusFailed) {
        return ImagePtr();
    }
    frameRenderArgs.updateNodesRequest(request);

    std::list<ImagePlaneDesc> planes;
    {
        ImagePlaneDesc plane, pairedPlane;
        effect->getMetadataComponents(-1, &plane, &pairedPlane);
        planes.push_back(plane);
    }
    std::unique_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs( time, scale, mipmapLevel, view,
                                                                                                  true, // byPassCache
                                                                                                  renderWindow, rod, planes,
                                                                                                  effect->getBitDepth(-1),
                                                                                                  false, // calledFromGetImage
                                                                                                  effect.get(),
                                                                                                  eStorageModeRAM, // returnStorage
                                                                                                  time ) ); // callerRenderTime
    std::map<ImagePlaneDesc, ImagePtr> outputPlanes;
    try {
        if ( (effect->renderRoI(*renderArgs, &outputPlanes) != EffectInstance::eRenderRoIRetCodeOk) || outputPlanes.empty() ) {
            return ImagePtr();
        }
    } catch (const std::exception& e) {
        std::cerr << "renderFrame: " << e.what() << std::endl;

        return ImagePtr();
    }

    return outputPlanes.begin()->second;
}
} // anonymous namespace

// ContextAttached fails: the frame is rendered on the CPU, and OpenGL is not tried again.
TEST(OfxOpenGLSupport, ContextAttachFailedRendersOnTheCPU)
{
    if ( !canRunTest(kNatronTestPluginGLAttachFailed, true) ) {
        return;
    }
    OpenGLRenderingSetter enabler(Settings::eEnableOpenGLEnabled);
    ASSERT_TRUE( appPTR->getCurrentSettings()->isOpenGLRenderingEnabled() );
    NodePtr node = createNode(kNatronTestPluginGLAttachFailed);
    ASSERT_TRUE( bool(node) );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, node->getCurrentOpenGLRenderSupport() );

    const int attach = NatronTestPlugin::counter("GLAttachFailed.attach");
    const int glRenders = NatronTestPlugin::counter("GLAttachFailed.glRenders");
    const int cpuRenders = NatronTestPlugin::counter("GLAttachFailed.cpuRenders");
    EXPECT_TRUE( bool( renderFrame(node, 1.) ) ) << "the render failed instead of falling back to the CPU";
    EXPECT_EQ( attach + 1, NatronTestPlugin::counter("GLAttachFailed.attach") );
    EXPECT_EQ( glRenders, NatronTestPlugin::counter("GLAttachFailed.glRenders") );
    EXPECT_EQ( cpuRenders + 1, NatronTestPlugin::counter("GLAttachFailed.cpuRenders") );
    EXPECT_EQ( ePluginOpenGLRenderSupportNone, node->getCurrentOpenGLRenderSupport() );

    // the next render does not try OpenGL again
    EXPECT_TRUE( bool( renderFrame(node, 2.) ) );
    EXPECT_EQ( attach + 1, NatronTestPlugin::counter("GLAttachFailed.attach") );
    EXPECT_EQ( cpuRenders + 2, NatronTestPlugin::counter("GLAttachFailed.cpuRenders") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("GLAttachFailed.detach") );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// ContextAttached fails for a plug-in that needs OpenGL: the render fails, and the next render does not
// attach again.
TEST(OfxOpenGLSupport, ContextAttachFailedNeeded)
{
    if ( !canRunTest(kNatronTestPluginGLAttachFailedNeeded, true) ) {
        return;
    }
    OpenGLRenderingSetter enabler(Settings::eEnableOpenGLEnabled);
    NodePtr node = createNode(kNatronTestPluginGLAttachFailedNeeded);
    ASSERT_TRUE( bool(node) );
    ASSERT_EQ( ePluginOpenGLRenderSupportNeeded, node->getCurrentOpenGLRenderSupport() );
    // OpenGL rendering is enabled: no "OpenGL unavailable" message
    EXPECT_TRUE( node->getOpenGLRenderUnavailableMessage().isEmpty() );

    const int attach = NatronTestPlugin::counter("GLAttachFailedNeeded.attach");
    const int cpuRenders = NatronTestPlugin::counter("GLAttachFailedNeeded.cpuRenders");
    EXPECT_FALSE( bool( renderFrame(node, 1.) ) );
    EXPECT_EQ( attach + 1, NatronTestPlugin::counter("GLAttachFailedNeeded.attach") );
    EXPECT_FALSE( bool( renderFrame(node, 2.) ) );
    EXPECT_EQ( attach + 1, NatronTestPlugin::counter("GLAttachFailedNeeded.attach") );
    EXPECT_EQ( cpuRenders, NatronTestPlugin::counter("GLAttachFailedNeeded.cpuRenders") );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// A plug-in that needs OpenGL, rendered where OpenGL rendering is disabled, fails with a message saying why.
TEST(OfxOpenGLSupport, NeededButDisabled)
{
    if ( !canRunTest(kNatronTestPluginGLAttachFailedNeeded, false) ) {
        return;
    }
    OpenGLRenderingSetter disabler(Settings::eEnableOpenGLDisabled);
    ASSERT_FALSE( appPTR->getCurrentSettings()->isOpenGLRenderingEnabled() );
    NodePtr node = createNode(kNatronTestPluginGLAttachFailedNeeded);
    ASSERT_TRUE( bool(node) );
    // the reason (set as a persistent message when it renders, which is only printed in background mode)
    EXPECT_FALSE( node->getOpenGLRenderUnavailableMessage().isEmpty() );

    // the render fails without calling the plug-in
    const int attach = NatronTestPlugin::counter("GLAttachFailedNeeded.attach");
    const int cpuRenders = NatronTestPlugin::counter("GLAttachFailedNeeded.cpuRenders");
    EXPECT_FALSE( bool( renderFrame(node, 1.) ) );
    EXPECT_EQ( attach, NatronTestPlugin::counter("GLAttachFailedNeeded.attach") );
    EXPECT_EQ( cpuRenders, NatronTestPlugin::counter("GLAttachFailedNeeded.cpuRenders") );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

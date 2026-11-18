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

#ifndef NATRON_TESTS_OFXTESTPLUGIN_H
#define NATRON_TESTS_OFXTESTPLUGIN_H

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

#include <ofxhBinary.h>
#include <ofxhParam.h>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/ParallelRenderArgs.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

// The OFX plug-ins of Tests/OfxTestPlugin/NatronTestPlugin.cpp, built with the tests (see
// Tests/CMakeLists.txt) and loaded by the host like any other OFX plug-in.
#define kNatronTestPluginParamInteract "net.natron.test.ParamInteract"
#define kNatronTestPluginSingleInstance "net.natron.test.SingleInstance"
#define kNatronTestPluginSyncPrivateData "net.natron.test.SyncPrivateData"
#define kNatronTestPluginParams "net.natron.test.Params"
#define kNatronTestPluginGenerator "net.natron.test.Generator"
#define kNatronTestPluginMultiPAR "net.natron.test.MultiPAR"
#define kNatronTestPluginClipGetImage "net.natron.test.ClipGetImage"
#define kNatronTestPluginSequential "net.natron.test.Sequential"
#define kNatronTestPluginSequentialStatus "net.natron.test.SequentialStatus"
#define kNatronTestPluginTimeBlur "net.natron.test.TimeBlur"
#define kNatronTestPluginGPURenderFailed "net.natron.test.GPURenderFailed"
#define kNatronTestPluginGPUOnlyRenderFailed "net.natron.test.GPUOnlyRenderFailed"
#define kNatronTestPluginQuestion "net.natron.test.Question"
#define kNatronTestPluginOverlayV1V2 "net.natron.test.OverlayV1V2"
#define kNatronTestPluginOverlayV2 "net.natron.test.OverlayV2"

namespace NatronTestPlugin {
// true if the host loaded the plug-in
inline bool
isAvailable(const char* pluginID)
{
    try {
        NATRON_NAMESPACE::Plugin* p = NATRON_NAMESPACE::appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getOfxPlugin();
    } catch (...) {
        return false;
    }
}

// The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
// so say clearly that they did not run.
inline void
reportSkipped(const char* pluginID)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": plug-in " << pluginID
              << " not found (the test plug-in built with the tests, NatronTestPlugin.ofx.bundle)" << std::endl;
}

#ifdef NATRON_TESTS_OFX_PLUGIN_BINARY
// The plug-in binary, which is the library the host loaded (see counter()).
inline OFX::Binary&
binary()
{
    static OFX::Binary binary(NATRON_TESTS_OFX_PLUGIN_BINARY);

    if ( !binary.isLoaded() ) {
        binary.load();
    }

    return binary;
}

#endif

// The value of a counter of the test plug-ins (NatronTestPluginGetCounter() in NatronTestPlugin.cpp),
// or -1 if the plug-in binary can't be loaded. Loading the binary that the host already loaded gives
// the same library, so these are the counters of the plug-ins the host uses.
inline int
counter(const std::string& name)
{
#ifdef NATRON_TESTS_OFX_PLUGIN_BINARY
    typedef int (*GetCounterFunc)(const char*);
    GetCounterFunc getCounter = (GetCounterFunc)binary().findSymbol("NatronTestPluginGetCounter");

    return getCounter ? getCounter( name.c_str() ) : -1;
#else
    (void)name;

    return -1;
#endif
}

// The OFX parameter instance of a parameter of an OFX node, whose handle (getHandle()) is what the
// plug-in gets from paramGetHandle(), or NULL.
inline OFX::Host::Param::Instance*
ofxParam(const NATRON_NAMESPACE::NodePtr& node,
         const std::string& name)
{
    NATRON_NAMESPACE::OfxEffectInstance* effect = node ? dynamic_cast<NATRON_NAMESPACE::OfxEffectInstance*>( node->getEffectInstance().get() ) : 0;

    return ( effect && effect->effectInstance() ) ? effect->effectInstance()->getParam(name) : 0;
}

// The OFX suites of the host, as the plug-ins get them from fetchSuite().
inline const OfxParameterSuiteV1*
parameterSuite()
{
    return (const OfxParameterSuiteV1*)OFX::Host::Param::GetSuite(1);
}

inline const OfxPropertySuiteV1*
propertySuite()
{
    return (const OfxPropertySuiteV1*)OFX::Host::Property::GetSuite(1);
}

// Set a counter of the test plug-ins (NatronTestPluginSetCounter() in NatronTestPlugin.cpp).
inline void
setCounter(const std::string& name,
           int value)
{
#ifdef NATRON_TESTS_OFX_PLUGIN_BINARY
    typedef void (*SetCounterFunc)(const char*, int);
    SetCounterFunc set = (SetCounterFunc)binary().findSymbol("NatronTestPluginSetCounter");

    if (set) {
        set(name.c_str(), value);
    }
#else
    (void)name;
    (void)value;
#endif
}

// Create a node of the top-level app's project, without undo/redo command and without connecting it.
inline NATRON_NAMESPACE::NodePtr
createNode(const char* pluginID)
{
    NATRON_NAMESPACE::AppInstancePtr app = NATRON_NAMESPACE::appPTR->getTopLevelInstance();
    NATRON_NAMESPACE::CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// Render the output of node at time, view 0 and scale 1, as an interactive render (like the preview
// render of Node::makePreviewImage()). allowGPURendering is passed to renderRoI(). Returns the
// rendered image, or a null image if the render failed.
inline NATRON_NAMESPACE::ImagePtr
renderFrame(const NATRON_NAMESPACE::NodePtr& node,
            double time,
            bool allowGPURendering = true)
{
    NATRON_NAMESPACE_USING
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
                                                                                                  false, // byPassCache
                                                                                                  renderWindow, rod, planes,
                                                                                                  effect->getBitDepth(-1),
                                                                                                  false, // calledFromGetImage
                                                                                                  effect.get(),
                                                                                                  eStorageModeRAM, // returnStorage
                                                                                                  time ) ); // callerRenderTime
    renderArgs->allowGPURendering = allowGPURendering;
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

// Render the frame range of output, as a render on disk (AppInstance::startWritersRendering(), which
// blocks in background mode, as in the tests).
inline void
renderSequence(const NATRON_NAMESPACE::NodePtr& output,
               int firstFrame,
               int lastFrame)
{
    NATRON_NAMESPACE_USING
    std::list<AppInstance::RenderWork> works;
    AppInstance::RenderWork w( dynamic_cast<OutputEffectInstance*>( output->getEffectInstance().get() ), firstFrame, lastFrame, 1, false );
    if (w.writer) {
        works.push_back(w);
        output->getApp()->startWritersRendering(false, works);
    }
}
} // namespace NatronTestPlugin

#endif // NATRON_TESTS_OFXTESTPLUGIN_H

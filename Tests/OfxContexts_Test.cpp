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

// Plug-in contexts (#1383): "kOfxImageEffectActionDescribeInContext will be called once for each
// context that the host and plug-in mutually support. If a plug-in does not report to support any
// context supported by host, host should not enable the plug-in." (ofxCore.h), and a context whose
// DescribeInContext returns kOfxStatErrMissingHostFeature "will be ignored by the host"
// (ofxImageEffect.h).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <ofxImageEffect.h>
#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginContexts.cpp
#define kNatronTestPluginRetimerOnly "net.natron.test.RetimerOnly"
#define kNatronTestPluginRetimerOrFilter "net.natron.test.RetimerOrFilter"
#define kNatronTestPluginContextFallback "net.natron.test.ContextFallback"

NATRON_NAMESPACE_USING

namespace {
// the context the node was created in
std::string
createAndGetContext(const char* pluginID)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    if (!node) {
        return std::string();
    }
    OfxEffectInstance* effect = dynamic_cast<OfxEffectInstance*>( node->getEffectInstance().get() );
    std::string context = (effect && effect->effectInstance()) ? effect->effectInstance()->getContext() : std::string();
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);

    return context;
}
} // anonymous namespace

TEST(OfxContexts, SupportedContexts)
{
    std::set<std::string> contexts;

    // Retimer is not supported, unknown contexts neither
    contexts.insert(kOfxImageEffectContextRetimer);
    contexts.insert("OfxImageEffectContextUnknown");
    EXPECT_TRUE( NATRON_NAMESPACE::OfxHost::getSupportedContexts(contexts).empty() );

    // by order of preference
    contexts.insert(kOfxImageEffectContextGenerator);
    contexts.insert(kOfxImageEffectContextFilter);
    contexts.insert(kOfxImageEffectContextGeneral);
    std::vector<std::string> supported = NATRON_NAMESPACE::OfxHost::getSupportedContexts(contexts);
    ASSERT_EQ( 3u, supported.size() );
    EXPECT_EQ(kOfxImageEffectContextGeneral, supported[0]);
    EXPECT_EQ(kOfxImageEffectContextFilter, supported[1]);
    EXPECT_EQ(kOfxImageEffectContextGenerator, supported[2]);

    contexts.clear();
    contexts.insert(kOfxImageEffectContextTransition);
    contexts.insert(kOfxImageEffectContextPaint);
    contexts.insert(kOfxImageEffectContextReader);
    contexts.insert(kNatronOfxImageEffectContextTracker);
    contexts.insert(kOfxImageEffectContextWriter);
    supported = NATRON_NAMESPACE::OfxHost::getSupportedContexts(contexts);
    ASSERT_EQ( 5u, supported.size() );
    EXPECT_EQ(kOfxImageEffectContextReader, supported[0]);
    EXPECT_EQ(kOfxImageEffectContextWriter, supported[1]);
    EXPECT_EQ(kNatronOfxImageEffectContextTracker, supported[2]);
    EXPECT_EQ(kOfxImageEffectContextPaint, supported[3]);
    EXPECT_EQ(kOfxImageEffectContextTransition, supported[4]);
}

// A plug-in whose only context is not supported is not loaded.
TEST(OfxContexts, UnsupportedPluginNotLoaded)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginRetimerOrFilter) ) {
        // the bundle was not loaded at all
        NatronTestPlugin::reportSkipped(kNatronTestPluginRetimerOrFilter);

        return;
    }
    EXPECT_FALSE( NatronTestPlugin::isAvailable(kNatronTestPluginRetimerOnly) );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RetimerOnly.describe." kOfxImageEffectContextRetimer) );
}

// A plug-in is described and created in a supported context only.
TEST(OfxContexts, UnsupportedContextNotDescribed)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginRetimerOrFilter) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginRetimerOrFilter);

        return;
    }
    EXPECT_EQ( std::string(kOfxImageEffectContextFilter), createAndGetContext(kNatronTestPluginRetimerOrFilter) );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RetimerOrFilter.describe." kOfxImageEffectContextRetimer) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("RetimerOrFilter.describe." kOfxImageEffectContextFilter) );
}

// A context whose description returns kOfxStatErrMissingHostFeature is skipped: the next one is used.
TEST(OfxContexts, MissingHostFeatureFallback)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginContextFallback) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginContextFallback);

        return;
    }
    EXPECT_EQ( std::string(kOfxImageEffectContextFilter), createAndGetContext(kNatronTestPluginContextFallback) );
    // The description is done when the first node of the plug-in is created, and kept for the process
    // (Plugin::setOfxDesc()): each context was described once, whichever test created the first node.
    const int describedGeneral = NatronTestPlugin::counter("ContextFallback.describe." kOfxImageEffectContextGeneral);
    const int describedFilter = NatronTestPlugin::counter("ContextFallback.describe." kOfxImageEffectContextFilter);
    EXPECT_EQ(1, describedGeneral);
    EXPECT_EQ(1, describedFilter);
    // a second node uses it: neither the failed context nor the good one is described again
    EXPECT_EQ( std::string(kOfxImageEffectContextFilter), createAndGetContext(kNatronTestPluginContextFallback) );
    EXPECT_EQ( describedGeneral, NatronTestPlugin::counter("ContextFallback.describe." kOfxImageEffectContextGeneral) );
    EXPECT_EQ( describedFilter, NatronTestPlugin::counter("ContextFallback.describe." kOfxImageEffectContextFilter) );
}

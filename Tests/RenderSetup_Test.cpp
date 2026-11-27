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

// The render args of a frame (ParallelRenderArgs), which the effects of a render keep on the thread-local storage
// while it lasts (#1846): renderRoI() called without them sets up its own, and must remove them when it returns, and a
// DiskCache node asks its input for its frame range with the hash of the input in the render.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>
#include <memory>

#include <gtest/gtest.h>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/ParallelRenderArgs.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

#include "OfxTestPlugin.h"
#include "TestUtils.h"

NATRON_NAMESPACE_USING

namespace {
bool
canRunTest()
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginGenerator);

        return false;
    }

    return true;
}

// renderRoI() on the effect of node, without any render args on this thread (no ParallelRenderArgsSetter, no request
// pass), as a plug-in or a script asking for an image outside a render would. Returns true if it rendered.
bool
renderWithoutRenderArgs(const NodePtr& node)
{
    const EffectInstancePtr effect = node->getEffectInstance();
    const double time = 1.;
    const ViewIdx view(0);
    const unsigned int mipmapLevel = 0;
    const RenderScale scale = RenderScale::fromMipmapLevel(mipmapLevel);
    RectD rod;
    if ( (effect->getRegionOfDefinition_public(effect->getHash(), time, scale, view, &rod, nullptr) == eStatusFailed) || rod.isNull() ) {
        return false;
    }
    std::list<ImagePlaneDesc> planes;
    {
        ImagePlaneDesc plane, pairedPlane;
        effect->getMetadataComponents(-1, &plane, &pairedPlane);
        planes.push_back(plane);
    }
    const EffectInstance::RenderRoIArgs args( time, scale, mipmapLevel, view,
                                              false, // byPassCache
                                              rod.toPixelEnclosing( mipmapLevel, effect->getAspectRatio(-1) ), rod, planes,
                                              effect->getBitDepth(-1),
                                              false, // calledFromGetImage
                                              effect.get(),
                                              eStorageModeRAM, // returnStorage
                                              time ); // callerRenderTime
    std::map<ImagePlaneDesc, ImagePtr> outputPlanes;

    return (effect->renderRoI(args, &outputPlanes) == EffectInstance::eRenderRoIRetCodeOk) && !outputPlanes.empty();
}
} // anon namespace

// renderRoI() without render args sets up its own (it says "[BUG]"): they stayed on the thread after it returned, so
// that the later renders and actions of the effect on that thread took them for the args of their render, with the
// hash the node had then.
TEST(RenderSetup, RenderRoIWithoutRenderArgsRemovesItsOwn)
{
    if ( !canRunTest() ) {
        return;
    }
    NatronTest::ProjectResetScope projectReset;
    NatronTest::NodeScope scope;
    const NodePtr generator = scope.create(kNatronTestPluginGenerator);
    ASSERT_TRUE( bool(generator) );
    const EffectInstancePtr effect = generator->getEffectInstance();
    ASSERT_FALSE( bool( effect->getParallelRenderArgsTLS() ) );

    EXPECT_TRUE( renderWithoutRenderArgs(generator) );
    EXPECT_FALSE( bool( effect->getParallelRenderArgsTLS() ) ) << "the render args of renderRoI() stayed on the thread";

    // a change of the node outside a render: its render hash is its new hash
    generator->incrementKnobsAge();
    EXPECT_EQ( effect->getHash(), effect->getRenderHash() );
}

// In "Input frame range" mode, DiskCacheNode::getFrameRange() asks its input for its frame range with the hash of the
// input in the render, under which the input caches it: it used the live hash, which changes as soon as a parameter
// changes, so that the frame range was computed again during the render, and cached under a hash that the input
// doesn't use in it.
TEST(RenderSetup, DiskCacheInputFrameRangeUsesTheRenderHash)
{
    if ( !canRunTest() ) {
        return;
    }
    NatronTest::ProjectResetScope projectReset;
    NatronTest::NodeScope scope;
    const NodePtr generator = scope.create(kNatronTestPluginGenerator);
    const NodePtr diskCache = scope.create(PLUGINID_NATRON_DISKCACHE);
    ASSERT_TRUE(generator && diskCache);
    ASSERT_TRUE( Project::connectNodes(0, generator, diskCache) );
    KnobChoicePtr frameRange = std::dynamic_pointer_cast<KnobChoice>( diskCache->getKnobByName("frameRange") );
    ASSERT_TRUE( bool(frameRange) );
    ASSERT_EQ( 0, frameRange->getValue() ) << "not \"Input frame range\"";

    // the render args of a render of the DiskCache node, as renderFrame() sets them up
    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    ParallelRenderArgsSetter renderArgs( 1., ViewIdx(0),
                                         true, // isRenderUserInteraction
                                         false, // isSequential
                                         abortInfo,
                                         diskCache, // tree root
                                         0, // texture index
                                         diskCache->getApp()->getTimeLine().get(),
                                         NodePtr(), // rotoPaint node
                                         false, // isAnalysis
                                         false, // isDraft
                                         RenderStatsPtr() );
    const EffectInstancePtr input = generator->getEffectInstance();
    double first = 0., last = 0.;
    // cached under the render hash of the input
    input->getFrameRange_public(input->getRenderHash(), &first, &last);

    // a parameter of the input changes during the render
    generator->incrementKnobsAge();
    ASSERT_NE( input->getHash(), input->getRenderHash() );

    const int timeDomainsBefore = NatronTestPlugin::counter("Generator.getTimeDomain");
    const EffectInstancePtr diskCacheEffect = diskCache->getEffectInstance();
    diskCacheEffect->getFrameRange_public(diskCacheEffect->getRenderHash(), &first, &last, true /*bypasscache*/);
    EXPECT_EQ( 0, NatronTestPlugin::counter("Generator.getTimeDomain") - timeDomainsBefore )
        << "the frame range of the input was computed again, for its hash outside the render";
}

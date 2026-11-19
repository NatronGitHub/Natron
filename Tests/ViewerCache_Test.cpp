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

// Tests of the invalidation of the viewer cache (the textures displayed by the viewers):
// - #1119: "Clear Playback Cache" removes the cached viewer textures;
// - #1115: Refresh removes the cached textures of the current frame;
// - #1116: changing the project LUTs invalidates the cached images and textures.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/FrameEntry.h"
#include "Engine/FrameKey.h"
#include "Engine/FrameParams.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/TextureRect.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createDot()
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args(PLUGINID_NATRON_DOT, app->getProject());
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);

    return app->createNode(args);
}
} // anonymous namespace

// #1119: the viewer cache is tiled, so its entries are always in its disk portion: "Clear Playback Cache"
// (AppManager::clearPlaybackCache()) must remove them, not only clear the (always empty) memory portion.
TEST(ViewerCache, ClearPlaybackCacheRemovesTextures)
{
    NodePtr holder = createDot();
    ASSERT_TRUE( bool(holder) );

    const RectI bounds(0, 0, 16, 16);
    FrameKey key(holder.get(), 1, 0x7777777777777777ULL, 1., 1., (int)eViewerColorSpaceSRGB, (int)eImageBitDepthByte,
                 (int)eDisplayChannelsRGB, ViewIdx(0), TextureRect(0, 0, 16, 16, 1, 1.), 0, "Input1",
                 ImagePlaneDesc::getRGBAComponents(), std::string(), false, false);
    {
        FrameParamsPtr params = std::make_shared<FrameParams>(bounds, (int)eImageBitDepthByte, bounds, ImagePtr());
        FrameEntryPtr entry;
        ASSERT_FALSE( appPTR->getTextureOrCreate(key, params, NULL, &entry) );
        ASSERT_TRUE( bool(entry) );
        entry->allocateMemory();
    }
    {
        std::list<FrameEntryPtr> found;
        ASSERT_TRUE( appPTR->getTexture(key, &found) );
    }

    appPTR->clearPlaybackCache();

    {
        std::list<FrameEntryPtr> found;
        EXPECT_FALSE( appPTR->getTexture(key, &found) );
    }
    holder->destroyNode(true, false);
}

// #1115: Refresh removes the cached textures of the current frame of the viewer (the forced render neither uses nor
// fills the texture cache, so they would come back at the next pan or zoom), and keeps those of the other frames.
TEST(ViewerCache, RemoveTexturesOfOneFrame)
{
    NodePtr holder = createDot();
    ASSERT_TRUE( bool(holder) );

    const RectI bounds(0, 0, 16, 16);
    std::vector<FrameKey> keys;
    for (int time = 1; time <= 2; ++time) {
        keys.push_back( FrameKey(holder.get(), time, 0x8888888888888888ULL, 1., 1., (int)eViewerColorSpaceSRGB, (int)eImageBitDepthByte,
                                 (int)eDisplayChannelsRGB, ViewIdx(0), TextureRect(0, 0, 16, 16, 1, 1.), 0, "Input1",
                                 ImagePlaneDesc::getRGBAComponents(), std::string(), false, false) );
        FrameParamsPtr params = std::make_shared<FrameParams>(bounds, (int)eImageBitDepthByte, bounds, ImagePtr());
        FrameEntryPtr entry;
        ASSERT_FALSE( appPTR->getTextureOrCreate(keys.back(), params, NULL, &entry) );
        ASSERT_TRUE( bool(entry) );
        entry->allocateMemory();
    }

    appPTR->removeAllTexturesFromCacheForHolderAtTime(holder.get(), 1);

    std::list<FrameEntryPtr> found;
    EXPECT_FALSE( appPTR->getTexture(keys[0], &found) );
    found.clear();
    EXPECT_TRUE( appPTR->getTexture(keys[1], &found) );
    found.clear();
    appPTR->clearPlaybackCache();
    holder->destroyNode(true, false);
}

// #1116: the project LUTs (Project Settings > LUT) are used to convert the images between bit depths, in the nodes
// and in the viewer, but they are neither in the node hash nor in the viewer cache key: changing them must change
// the hash of the nodes, so that the cached images and textures computed with the previous LUTs are not used.
TEST(ViewerCache, ProjectLutChangeChangesNodeHash)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    NodePtr node = createDot();
    ASSERT_TRUE( bool(node) );

    const char* lutKnobs[] = { "defaultColorSpace8u", "defaultColorSpace16u", "defaultColorSpace32f" };
    for (int i = 0; i < 3; ++i) {
        KnobChoicePtr lut = std::dynamic_pointer_cast<KnobChoice>( app->getProject()->getKnobByName(lutKnobs[i]) );
        ASSERT_TRUE( bool(lut) ) << lutKnobs[i];
        const int oldValue = lut->getValue();
        const U64 oldHash = node->getHashValue();
        lut->setValue(oldValue == 0 ? 1 : 0);
        EXPECT_NE( oldHash, node->getHashValue() ) << lutKnobs[i];
        lut->setValue(oldValue);
    }
    node->destroyNode(true, false);
}

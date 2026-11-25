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

// Tests of the cache IDs of the nodes, which identify the owner of each cache entry:
// - #1848: the asynchronous removal of the cache entries of a destroyed node must not remove those of a node
//   created afterwards with the same name.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/FrameEntry.h"
#include "Engine/FrameKey.h"
#include "Engine/FrameParams.h"
#include "Engine/Image.h"
#include "Engine/ImageKey.h"
#include "Engine/ImagePlaneDesc.h"
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

// The owner of the cache entries of a destroyed node: the requests of the cleaner threads of the caches only
// carry the cache ID of their holder.
class DestroyedHolder
    : public CacheEntryHolder
{
public:
    explicit DestroyedHolder(const std::string& cacheID)
        : _cacheID(cacheID)
    {
    }

    virtual std::string getCacheID() const OVERRIDE FINAL
    {
        return _cacheID;
    }

private:
    std::string _cacheID;
};

ImageParamsPtr
makeImageParams()
{
    // In RAM, in both image caches: the cleanup only looks at the keys
    return Image::makeParams(RectD(0., 0., 16., 16.), 1., 0, false, ImagePlaneDesc::getRGBAComponents(), eImageBitDepthByte,
                             eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, eStorageModeRAM);
}
} // anonymous namespace

// #1848: destroying a node queues the removal of its entries from the node cache, the disk cache and the viewer
// cache (Node::destroyNode() -> removeAllImagesFromCache(false)), which the cleaner thread of each cache runs
// later. The request is keyed by the cache ID of the node. A node created afterwards with the same name must get
// another cache ID, so that the request can't remove its entries, however late it runs.
TEST(CacheHolderID, CleanupOfDestroyedNodeKeepsEntriesOfNewNodeWithSameName)
{
    // No request left by the previous tests
    appPTR->waitForCacheThreads();

    NodePtr destroyed = createDot();
    ASSERT_TRUE( bool(destroyed) );
    const std::string scriptName = destroyed->getScriptName();
    const std::string destroyedCacheID = destroyed->getCacheID();
    destroyed->destroyNode(true, false);
    destroyed.reset();

    NodePtr node = createDot();
    ASSERT_TRUE( bool(node) );
    // The new node gets the name of the destroyed one
    ASSERT_EQ( scriptName, node->getScriptName() );

    const ImageKey imageKey(node.get(), 0x1848184818481848ULL, false, 0., ViewIdx(0), 1., false, false);
    {
        ImagePtr image;
        ASSERT_FALSE( appPTR->getImageOrCreate(imageKey, makeImageParams(), &image) );
        ASSERT_TRUE( bool(image) );
        image->allocateMemory();
    }
    {
        ImagePtr image;
        ASSERT_FALSE( appPTR->getImageOrCreate_diskCache(imageKey, makeImageParams(), &image) );
        ASSERT_TRUE( bool(image) );
        image->allocateMemory();
    }
    const RectI bounds(0, 0, 16, 16);
    const FrameKey frameKey(node.get(), 1, 0x1848184818481848ULL, 1., 1., (int)eViewerColorSpaceSRGB, (int)eImageBitDepthByte,
                            (int)eDisplayChannelsRGB, ViewIdx(0), TextureRect(0, 0, 16, 16, 1, 1.), 0, "Input1",
                            ImagePlaneDesc::getRGBAComponents(), std::string(), false, false);
    {
        FrameParamsPtr params = std::make_shared<FrameParams>(bounds, (int)eImageBitDepthByte, bounds, ImagePtr());
        FrameEntryPtr texture;
        ASSERT_FALSE( appPTR->getTextureOrCreate(frameKey, params, NULL, &texture) );
        ASSERT_TRUE( bool(texture) );
        texture->allocateMemory();
    }

    // The request queued by destroyNode() may already have run: queue it again, as if the cleaner threads ran it
    // only now. It is the same request: the cache ID of the destroyed node, all its entries.
    const DestroyedHolder destroyedHolder(destroyedCacheID);
    appPTR->removeAllCacheEntriesForHolder(&destroyedHolder, false);
    appPTR->waitForCacheThreads();

    std::list<ImagePtr> images;
    EXPECT_TRUE( appPTR->getImage(imageKey, &images) ) << "node cache, cache ID " << node->getCacheID();
    images.clear();
    EXPECT_TRUE( appPTR->getImage_diskCache(imageKey, &images) ) << "disk cache, cache ID " << node->getCacheID();
    images.clear();
    std::list<FrameEntryPtr> textures;
    EXPECT_TRUE( appPTR->getTexture(frameKey, &textures) ) << "viewer cache, cache ID " << node->getCacheID();
    textures.clear();

    node->destroyNode(true, false);
    appPTR->waitForCacheThreads();
}

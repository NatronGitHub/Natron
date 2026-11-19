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

// Tests of the in-memory image cache (the cache used for the images of the nodes):
// - #1129: Cache::removeEntry(entry) removes that entry, not another one with an equal key;
// - #1130: removing the entries of a node keeps the LRU order of the other entries.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Image.h"
#include "Engine/ImageKey.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
typedef Cache<Image> ImageCache;
typedef std::shared_ptr<ImageCache> ImageCachePtr;

NodePtr
createDot()
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args(PLUGINID_NATRON_DOT, app->getProject());
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);

    return app->createNode(args);
}

// An in-memory cache, like the NodeCache (see AppManager::initializeCaches())
class ImageCacheTest
    : public testing::Test
{
protected:
    virtual void SetUp() OVERRIDE
    {
        _holderA = createDot();
        ASSERT_TRUE( bool(_holderA) );
        _holderB = createDot();
        ASSERT_TRUE( bool(_holderB) );
        std::string cacheName = std::string("TestImageCache") + QString::number( QCoreApplication::applicationPid() ).toStdString();
        _cache = std::make_shared<ImageCache>(cacheName, NATRON_CACHE_VERSION, 1024ULL * 1024ULL * 1024ULL, 1.);
    }

    virtual void TearDown() OVERRIDE
    {
        if (_cache) {
            _cache->waitForDeleterThread();
            _cache.reset();
        }
        if (_holderA) {
            _holderA->destroyNode(true, false);
        }
        if (_holderB) {
            _holderB->destroyNode(true, false);
        }
    }

    static ImageKey makeKey(const NodePtr& holder,
                            U64 nodeHash)
    {
        return ImageKey(holder.get(), nodeHash, false, 0., ViewIdx(0), 1., false, false);
    }

    ImagePtr createImage(const ImageKey& key,
                         unsigned int mipmapLevel) const
    {
        const double s = 16. * (1 << mipmapLevel);
        ImageParamsPtr params = Image::makeParams(RectD(0., 0., s, s), 1., mipmapLevel, false,
                                                  ImagePlaneDesc::getRGBAComponents(), eImageBitDepthByte,
                                                  eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, eStorageModeRAM);
        ImagePtr image;
        _cache->getOrCreate(key, params, NULL, &image);

        return image;
    }

    bool isInCache(const ImagePtr& image) const
    {
        std::list<ImagePtr> entries;
        _cache->getCopy(&entries);

        return std::find(entries.begin(), entries.end(), image) != entries.end();
    }

    std::list<U64> hashesInCache() const
    {
        std::list<ImagePtr> entries;
        _cache->getCopy(&entries);
        std::list<U64> ret;
        for (std::list<ImagePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
            ret.push_back( (*it)->getHashKey() );
        }

        return ret;
    }

    NodePtr _holderA, _holderB;
    ImageCachePtr _cache;
};
} // anonymous namespace

// #1129: ImageKey has no mipmap level, so the images of a frame at two mipmap levels have equal keys.
// removeEntry() must remove the given image, not the first image of the bucket with an equal key.
TEST_F(ImageCacheTest, RemoveEntryRemovesThatEntry)
{
    const ImageKey key = makeKey(_holderA, 0x5555555555555555ULL);
    ImagePtr fullScale = createImage(key, 0);
    ImagePtr halfScale = createImage(key, 1);
    ASSERT_TRUE( bool(fullScale) );
    ASSERT_TRUE( bool(halfScale) );
    ASSERT_NE(fullScale, halfScale);
    ASSERT_TRUE( fullScale->getKey() == halfScale->getKey() );
    ASSERT_TRUE( isInCache(fullScale) );
    ASSERT_TRUE( isInCache(halfScale) );

    _cache->removeEntry(halfScale);

    EXPECT_FALSE( isInCache(halfScale) );
    EXPECT_TRUE( isInCache(fullScale) );
}

// #1130: removing the entries of a node (as done on every change of a node hash) must keep the LRU order of the
// entries of the other nodes, so that the least recently used ones are still evicted first.
TEST_F(ImageCacheTest, RemovingEntriesOfANodeKeepsLRUOrder)
{
    const int n = 16;
    std::vector<U64> hashesInLRUOrder;
    for (int i = 0; i < n; ++i) {
        ImagePtr image = createImage(makeKey(_holderB, 0x1000 + i), 0);
        ASSERT_TRUE( bool(image) );
        hashesInLRUOrder.push_back( image->getHashKey() );
        // An image of the other node after each one
        ASSERT_TRUE( bool( createImage(makeKey(_holderA, 0x2000 + i), 0) ) );
    }
    ASSERT_EQ( (std::size_t)(2 * n), hashesInCache().size() );

    // Removes all the entries of _holderA, synchronously
    _cache->removeAllEntriesForHolderPublic(_holderA.get(), true);
    ASSERT_EQ( (std::size_t)n, hashesInCache().size() );

    // The entries of _holderB must be evicted in the order they were created (least recently used first)
    for (int i = 0; i < n; ++i) {
        ASSERT_TRUE( _cache->evictLRUInMemoryEntry() );
        std::list<U64> left = hashesInCache();
        EXPECT_EQ( (std::size_t)(n - i - 1), left.size() );
        EXPECT_TRUE( std::find(left.begin(), left.end(), hashesInLRUOrder[i]) == left.end() ) << "entry " << i << " was not evicted";
    }
}

// #1350: the deleter and cleaner threads of the cache must be finished before the cache, which owns
// their QThread, is destroyed, else Qt aborts the process with "QThread: Destroyed while thread is
// still running". This happened at exit, in ~AppManager: waitForDeleterThread() returned before
// run() had returned. Destroying a cache without calling waitForDeleterThread() must be safe too.
TEST_F(ImageCacheTest, DestroyCacheWhileItsThreadsRun)
{
    const std::string cacheName = std::string("TestImageCacheThreads") + QString::number( QCoreApplication::applicationPid() ).toStdString();
    for (int i = 0; i < 200; ++i) {
        _cache = std::make_shared<ImageCache>(cacheName, NATRON_CACHE_VERSION, 1024ULL * 1024ULL * 1024ULL, 1.);
        ImagePtr image = createImage(makeKey(_holderA, 0x3000 + i), 0);
        ASSERT_TRUE( bool(image) );
        ASSERT_TRUE( bool( createImage(makeKey(_holderB, 0x4000 + i), 0) ) );
        // Starts the deleter thread
        _cache->removeEntry(image);
        image.reset();
        // Starts the cleaner thread, which gives the entry of _holderB to the deleter thread
        _cache->removeAllEntriesWithDifferentNodeHashForHolderPublic(_holderB.get(), 0);
        if (i % 2 == 0) {
            // As in ~AppManager
            _cache->waitForDeleterThread();
        }
        _cache.reset();
    }
}
